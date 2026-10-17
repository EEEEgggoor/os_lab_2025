#include <unistd.h>    
#include <sys/types.h> 
#include <sys/wait.h>  
#include <stdlib.h>    
#include <stdio.h>     
#include <errno.h>     

int main(int argc, char **argv){

    pid_t pid = fork();
    

    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    
    if(pid == 0){
        printf("child pid %ld finish\n", (long)getpid());
        fflush(stdout);
        _exit(0);
    }

    if(argc == 1){ // ps -eo pid,ppid,stat,cmd - проверка
        int status;


        if(waitpid(pid, &status, 0)==-1){
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if(WIFEXITED(status)){
            printf("fork finished with code - %d\n", WEXITSTATUS(status));
        }
    } else {
    printf("Parent PID: %ld\n", (long)getpid());
    printf("Child PID: %ld\n", (long)pid);

    sleep(30);
    }
    

    return 0;
}
