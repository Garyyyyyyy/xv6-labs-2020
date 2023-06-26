//
// Created by Gary on 2023/6/21.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


int matchhere(char*, char*);
int matchstar(int, char*, char*);
int match(char*, char*);

char*
find_fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
    return buf;
}

void
find_fdpath(char *path,char *pattern)
{
//    printf("%s %s\n",path,pattern);
    char point[1]={'.'},dpoint[2]={'.','.'};
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:
//            printf("%s %d %d %l\n", path, st.type, st.ino, st.size);
            if(match(pattern, find_fmtname(path))) fprintf(1,"%s\n", path);
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
//            printf("%s %s %s %d\n",de,buf,path,DIRSIZ);
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                if(match(de.name,point)|| match(de.name,dpoint)) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                find_fdpath(buf,pattern);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc!=3){
        fprintf(2,"error!\n");
        exit(0);
    }
    find_fdpath(argv[1],argv[2]);
    exit(0);
}

int
match(char *re, char *text)
{
//    printf("%s %s %d\n",re,text,strcmp(re,text));
    return strcmp(re,text)==0;
}

