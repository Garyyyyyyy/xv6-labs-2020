//
// Created by Gary on 2023/6/21.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int f2c[2],c2f[2];
    pipe(f2c);
    pipe(c2f);

    char buf[5];

    int pid=fork();
    if(pid<0){
        fprintf(2, "fork failed!\n");
        exit(1);
    }
    if(pid==0){
        //son
        read(f2c[0],buf,1);
        fprintf(2,"%d: received p%sng\n",getpid(),buf);

        write(c2f[1],"o",1);
        close(c2f[0]),close(c2f[1]);
    }
    else{
        //father
        write(f2c[1],"i",1);
        close(f2c[0]),close(f2c[1]);

//        wait(NULL);

        read(c2f[0],buf,1);;
        fprintf(2,"%d: received p%sng\n",getpid(),buf);
        close(c2f[0]),close(c2f[1]);
    }
    exit(0);
}