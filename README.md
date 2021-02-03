# ksm-test
ksm(kennel same-page merge) 小测
## 测试系统
ubuntu 20.04
### 环境要求
```
sudo apt install ksmtuned
sudo apt install clang

grep CONFIG_KSM /boot/config-*
结果是 CONFIG_KSM=y

```
### ksm 配置
[参考资料](https://dannyda.com/2020/12/10/how-to-tune-ksm-kernel-samepage-merging-sharing-in-proxmox-ve-pve/?__cf_chl_captcha_tk__=47bd04826fff0e33459b5f5b26a858ab6a907b46-1612163667-0-Adx0-lZgAz-KxGJV9crqUmM97mg4zU-gno8XotgqV1FXEgBcPevvrBlKFHr8ylxrm-7bANfzuuTxcWKTf-LaNLJ2SvtOhiM-rHQulbeIDFT-4mGikMhWid_Eoo6xVf58lpzDJQLhv9mmDaA-qT5bqG1FgbWu9pa9s7UwD7Imniqfz_Dm3jC9nDlGqus81FohbtPD5WT2reZ2QSRWuS7b2acB7DeUqKp8S38oejjLuH4XZFpABcSzhHX9LuiV2F4fdVaGx961KhNgwss5s1TXkxllpnqyQKlRCSvwxpglPA1jl7IUL2HnSfRdhr65-_wkWHHDIUz7C1w7BFf9zPt3VXCAMvWj5em-kL5s4w-RYULiNYWPhqC87qdSkvMLsxO45eM_8bqRKYngtig0xzxdQmBXi1ISelivK3yzIIcNHJpSM8AzMjH3tjjIWrBPOcG9goq7r5xQiqMIIFpmcX636rl57dbnDafkno9I8Uv49BIeDTUp7OHzL2Y6XGKZcCjSkpf3S-Qypi_wieGEwsAoanJ91xoufCVUBiwLq20H5CXaXFE7CoG61r-Zbv4ixbrkVyP3fOpmBVaOb3qSLnga92YMF0eERdlyuu0FeViMVgLyd9pY6sT-zJUxkebOvrD13g) <br>
/etc/ksmtuned.conf <br>
[ksmtuned详解](ksmtuned.md) <br>
修改后重启服务 service ksmstuned restart
```
# Configuration file for ksmtuned.
# How long ksmtuned should sleep between tuning adjustments
# KSM_MONITOR_INTERVAL=60
  
# Millisecond sleep between ksm scans for 16Gb server.
# Smaller servers sleep more, bigger sleep less.
# KSM_SLEEP_MSEC=10
  
# KSM_NPAGES_BOOST - is added to the `npages` value, when `free memory` is less than `thres`.
# KSM_NPAGES_BOOST=300
  
# KSM_NPAGES_DECAY - is the value given is subtracted to the `npages` value, when `free memory` is greater than `thres`.
# KSM_NPAGES_DECAY=-50
  
# KSM_NPAGES_MIN - is the lower limit for the `npages` value.
# KSM_NPAGES_MIN=64
  
# KSM_NPAGES_MAX - is the upper limit for the `npages` value.
# KSM_NPAGES_MAX=1250
  
# KSM_THRES_COEF - is the RAM percentage to be calculated in parameter `thres`.
# KSM_THRES_COEF=20
  
# KSM_THRES_CONST - If this is a low memory system, and the `thres` value is less than `KSM_THRES_CONST`, then reset `thres` value to `KSM_THRES_CONST` value.
# KSM_THRES_CONST=2048
  
# uncomment the following to enable ksmtuned debug information
# LOGFILE=/var/log/ksmtuned
# DEBUG=1
```
monitor status配置<br>
/sys/kernel/mm/ksm <br>
[参考说明](https://www.kernel.org/doc/Documentation/vm/ksm.txt)

启用和停用
```
systemctl enable ksmtuned
systemctl disable ksmtuned
```
测试工程
```
编译
clang++ ksm.cpp -o ksm
分别启用
./ksm
./ksm 1
可以top观察内存变化，也可以观察 /sys/kernel/mm/ksm/ 下面的状态
```
