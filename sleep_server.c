#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

#define MANAGER_PATH "./manager"
#define MEMBER_PATH "./member"
#define MANAGER_FLAG "manager"
#define NO_ARGS 1
#define ONE_ARG 2

void tryExec(char *path);

int main(int argc, char **argv){
    if(argc == NO_ARGS){ 
        tryExec(MEMBER_PATH);
    } else if(strcmp(argv[1],MANAGER_FLAG) == 0 && argc == ONE_ARG){ 
        tryExec(MANAGER_PATH);
    } else{
        printf("Entrada errada");  
        exit(EXIT_FAILURE);
    }
    printf("Fim \n");  
    return 0;
}

void tryExec(char *path){
    char* args[] = { NULL, NULL };
    args[0] = path;
    if(execvp(args[0], args) == -1) 
        exit(EXIT_FAILURE);
}