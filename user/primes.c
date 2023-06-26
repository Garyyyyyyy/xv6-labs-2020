//
// Created by Gary on 2023/6/21.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void findprime(int *p);
int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    int pid=fork();
    if(pid<0){
        fprintf(2, "fork failed!\n");
        exit(1);
    }

    if(pid==0){
        findprime(p);
    }
    else{
        close(p[0]);
        for(int i=2;i<=35;i++){
            write(p[1],&i,sizeof (int));
        }
        close(p[1]);
        wait(0);
    }

    exit(0);
}
void findprime(int *p){

    close(p[1]);

    int prime;
    if(read(p[0], &prime, sizeof (int))==0){
        close(p[0]);
        exit(0);
    }
    printf("prime %d \n",prime);
    int np[2];
    pipe(np);
    int pid=fork();
    if (pid < 0) {
        fprintf(2, "fork failed!\n");
        exit(1);
    }
    if (pid > 0) {
        close(np[0]);
        int temp;
        while(read(p[0], &temp, sizeof (int))>0) {
            if(temp%prime!=0) write(np[1],&temp,sizeof(int));
        }
        close(p[0]);
        close(np[1]);
        wait(0);
    }
    else{
        close(p[0]);
        findprime(np);
    }
    wait(0);
}