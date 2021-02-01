#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
const int memSize = 1024*1024*300;

int main(int argc,char **argv){
        // mmap replace malloc/new
        char *ttt = (char*)mmap(nullptr,memSize,PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        char pidInfo[64]={0};
        for(int i=0;i<memSize-1;i++){
                ttt[i]=char(i);
        }
        int rtv = madvise(ttt,memSize,MADV_MERGEABLE);
        if(rtv){
                printf("madvise err %d,%d,%d",rtv,errno,int(errno==EINVAL));
                return 0;
        }
        if(argc>1){
                sleep(100);
                printf("change mem\n");
                //copy on write ,cancel merge
                for(int i=0;i<memSize-1;i++){
                        ttt[i]=char(i+1);
                }
                sleep(500);
        }else{
                sleep(600);
        }
        return 0;
}