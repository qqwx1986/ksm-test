# ksmtuned 解析
/usr/sbin/ksmtuned 是个bash脚本，我这边分析一下这个脚本
```
#!/bin/bash
#
# Copyright 2009 Red Hat, Inc. and/or its affiliates.
# Released under the GPL
#
# Author:      Dan Kenigsberg <danken@redhat.com>
#
# ksmtuned - a simple script that controls whether (and with what vigor) ksm
# should search for duplicated pages.
#
# starts ksm when memory commited to qemu processes exceeds a threshold, and
# make ksm work harder and harder untill memory load falls below that
# threshold.
#
# send SIGUSR1 to this process right after a new qemu process is started, or
# following its death, to retune ksm accordingly
#
# needs testing and ironing. contact danken@redhat.com if something breaks.

# 如果 ksmtuned.conf 存在，那么 source 这个
if [ -f /etc/ksmtuned.conf ]; then
    . /etc/ksmtuned.conf   
fi

# 这是个log函数
debug() {
    if [ -n "$DEBUG" ]; then
        s="`/bin/date`: $*"
        [ -n "$LOGFILE" ] && echo "$s" >> "$LOGFILE" || echo "$s"
    fi
}

# 如果 KSM_MONITOR_INTERVAL 在 ksmtuned.conf 中未定义那么默认值为 60
KSM_MONITOR_INTERVAL=${KSM_MONITOR_INTERVAL:-60} 
# 如果 KSM_NPAGES_BOOST 在 ksmtuned.conf 中未定义那么默认值为 300
KSM_NPAGES_BOOST=${KSM_NPAGES_BOOST:-300}
# 如果 KSM_NPAGES_DECAY 在 ksmtuned.conf 中未定义那么默认值为 -50
KSM_NPAGES_DECAY=${KSM_NPAGES_DECAY:--50}

# 如果 KSM_NPAGES_MIN 在 ksmtuned.conf 中未定义那么默认值为 64
KSM_NPAGES_MIN=${KSM_NPAGES_MIN:-64}
# 如果 KSM_NPAGES_MAX 在 ksmtuned.conf 中未定义那么默认值为 1250
KSM_NPAGES_MAX=${KSM_NPAGES_MAX:-1250}

# millisecond sleep between ksm scans for 16Gb server. Smaller servers sleep
# more, bigger sleep less.
# 如果 KSM_SLEEP_MSEC 在 ksmtuned.conf 中未定义那么默认值为 10
KSM_SLEEP_MSEC=${KSM_SLEEP_MSEC:-10}

# 如果 KSM_THRES_COEF 在 ksmtuned.conf 中未定义那么默认值为 20
KSM_THRES_COEF=${KSM_THRES_COEF:-20}
# 如果 KSM_THRES_CONST 在 ksmtuned.conf 中未定义那么默认值为 2048
KSM_THRES_CONST=${KSM_THRES_CONST:-2048}

# 获取当前系统的总内存数（单位kB）
total=`awk '/^MemTotal:/ {print $2}' /proc/meminfo`
# 日志记录
debug total $total

npages=0
# sleep时间是 16G/当前总内存， 所以总内存越大，sleep越短，反之内存越小，sleep越长，上面有英文的官方注释
sleep=$[KSM_SLEEP_MSEC * 16 * 1024 * 1024 / total]
# 如果 sleep <= 10，那么取10
[ $sleep -le 10 ] && sleep=10
# 日志记录
debug sleep $sleep
# 计算触发阙值 总内存*阙值比例(百分比)
thres=$[total * KSM_THRES_COEF / 100]
# 如果配置参数 $KSM_THRES_CONST > thres，那么thres就取值$KSM_THRES_CONST，这应该是保证系统最低的内存吧
if [ $KSM_THRES_CONST -gt $thres ]; then
    thres=$KSM_THRES_CONST
fi
# 日志记录
debug thres $thres

# ksm控制函数
KSMCTL () {
    case x$1 in
        xstop) 
            # 关闭 ksm
            echo 0 > /sys/kernel/mm/ksm/run
            ;;
        xstart)
            echo $2 > /sys/kernel/mm/ksm/pages_to_scan
            echo $3 > /sys/kernel/mm/ksm/sleep_millisecs
            # 打开 ksm
            echo 1 > /sys/kernel/mm/ksm/run
            ;;
    esac
}

# kvm相关进程提交的内存（如果不是kvm相关用不到，比较这个ksm最开始是给kvm用的）
committed_memory () {
    local pidlist
    pidlist=$(pgrep -f -d ' ' '^qemu-system.*-enable-kvm')
    if [ -n "$pidlist" ]; then
        ps -p "$pidlist" -o rsz=
    fi | awk '{ sum += $1 }; END { print 0+sum }'
}

# 系统空闲内存(kB)
free_memory () {
    awk '/^(MemFree|Buffers|Cached):/ {free += $2}; END {print free}' \
                /proc/meminfo
}

# 增加 npages 范围是 [$KSM_NPAGES_MIN, $KSM_NPAGES_MAX]
increase_npages() {
    local delta
    delta=${1:-0}
    npages=$[npages + delta]
    if [ $npages -lt $KSM_NPAGES_MIN ]; then
        npages=$KSM_NPAGES_MIN
    elif [ $npages -gt $KSM_NPAGES_MAX ]; then
        npages=$KSM_NPAGES_MAX
    fi
    echo $npages
}

# 调整
adjust () {
    local free committed
    free=`free_memory`
    committed=`committed_memory`
    debug committed $committed free $free
    # 提交+阙值 小于 总内存 且 可用内存 大于 阙值， stop ksm
    if [ $[committed + thres] -lt $total -a $free -gt $thres ]; then
        KSMCTL stop
        debug "$[committed + thres] < $total and free > $thres, stop ksm"
        return 1
    fi
    debug "$[committed + thres] > $total, start ksm"
    if [ $free -lt $thres ]; then
        # 当前free < 阙值 npages += $KSM_NPAGES_BOOST
        npages=`increase_npages $KSM_NPAGES_BOOST`
        debug "$free < $thres, boost"
    else
        # free >= 阙值 npages -= |$KSM_NPAGES_DECAY|
        npages=`increase_npages $KSM_NPAGES_DECAY`
        debug "$free > $thres, decay"
    fi
    # 总结就是剩余内存越小 pages_to_scan 越大，随着merge时，剩余内存越大 pages_to_scan就越小直到为0
    KSMCTL start $npages $sleep
    debug "KSMCTL start $npages $sleep"
    return 0
}

function nothing () {
    :
}
# 循环检测
loop () {
    trap nothing SIGUSR1
    while true
    do
        sleep $KSM_MONITOR_INTERVAL &
        wait $!
        adjust
    done
}

PIDFILE=${PIDFILE-/var/run/ksmtune.pid}
if touch "$PIDFILE"; then
  loop &
  echo $! > "$PIDFILE"
fi
```
总结这个脚本就是根据/etc/ksmtuned.conf 的配置以及当前系统内存的使用情况，<br>
改变 /sys/kernel/mm/ksm/ 目录下的 run, pages_to_scan, sleep_millisecs的配置 <br>
ksmd 进程会根据上面三个配置做相应的逻辑处理
