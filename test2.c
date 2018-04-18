#include "types.h"
#include "stat.h"
#include "user.h"

#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19


void handler_user_1(int signum)
{
    int j;
    for(j=0 ; j < 500/*LARGE*/ ; j++)
    {
        printf(1,"   %d",j);
    }
    printf(1,"end user func 1\n");

}


void handler_user_2(int signum)
{
    int j;
    for(j=0 ; j < 20/*LARGE*/ ; j++)
    {
        printf(1,"Amazing");
    }
    printf(1,"\nend user func 2\n");
}

void handler_user_3(int signum)
{
    int j;
    for(j=0 ; j < 20/*LARGE*/ ; j++)
    {
        printf(1,"Last but not least!!!");
    }
    printf(1,"\nend user func 3\n");
}

int
main(void)
{
//     int i;
// int waistingTime=1;
    int parent = getpid();
int pid= fork();
if (pid==0){
    
    int j;
    for(j=0 ; j < 1000/*LARGE*/ ; j++)
    {
        printf(1,"   %d",j);
    }
    printf(1,"end user func CHILD\n");

    

}
else{

    sleep(50);
    kill(pid,SIGSTOP);
    sleep(500);
    kill(pid,SIGCONT);
    printf(1,"dady after\n");
    sleep(200);

}    

    int j;
    if (parent==getpid()){
        for(j=0; j<1;j++){
            wait();
            sleep(2000);
        }
        exit();
    }
    else{
        printf(1,"child with pid: %d finish to run\n",getpid());
        exit() ;        
    }
}

