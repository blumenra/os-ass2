#include "types.h"
#include "stat.h"
#include "user.h"

#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

void blaTest(int i);
void test1(void);
void test2(void);
void test3(void);
void test4(void);

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

/*ORIGINAL TEST*/
// int
// main(void)
// {
// //     int i;
// // int waistingTime=1;
//     int parent = getpid();
//     int pid= fork();
//     if (pid==0){
        
//         int j;
//         for(j=0 ; j < 1000/*LARGE*/ ; j++)
//         {
//             printf(1,"   %d",j);
//         }
//         printf(1,"end user func CHILD\n");
//         exit();
//     }

//     sleep(50);
//     kill(pid,SIGSTOP);
//     sleep(500);
//     kill(pid,SIGCONT);
//     printf(1,"dady after\n");
//     sleep(200);

//     int j;
//     if (parent==getpid()){
//         for(j=0; j<1;j++){
//             wait();
//             sleep(2000);
//         }
//         exit();
//     } else{
//         printf(1,"child with pid: %d finish to run\n",getpid());
//         exit() ;        
//     }
// }

void
test1(void)
{
    sigprocmask(8);
    sigprocmask(4);
    //in this point father's 4 and 8 masks should be turned on

    signal(11, blaTest);
    signal(12, blaTest);
    //in this point father's 11 and 12 signals should be mapped to blaTest

    //kill(getpid(), 6);
    kill(getpid(), 19);

    // int sonPid = fork();
    // if(sonPid == 0){
    //     //son running
    //     printf(1, "SON is going to sleep..\n");
    //     sleep(500);
    //     printf(1, "SON woke up!\n");

    //     exit();
    // }

    printf(1, "FATHER's pid: %d\n", getpid());
    // printf(1, "SON's pid: %d\n", sonPid);
    

    //father runnin
    printf(1, "FATHER is going to sleep..\n");
    sleep(500);
    // wait();
    // printf(1, "FATHER woke up!\n");

    // exit();
}

void
test2(void)
{

    int sonPid = fork();
    if(sonPid == 0){
        //son running
        for(int j=0 ; j < 1000/*LARGE*/ ; j++)
        {
            printf(1,"   %d",j);
        }

        sleep(500);

        if(fork() == 0){
            //grandson running

            printf(1, "GRANDSON is going to sleep...\n");
            sleep(500);
            
            exit();
        }

        //son running
        printf(1, "SON is going to sleep...\n");
        sleep(500);

        exit();
    }

    printf(1, "FATHER's pid: %d\n", getpid());
    // printf(1, "SON's pid: %d\n", sonPid);
    
    kill(sonPid, 15);

    //father runnin
    printf(1, "FATHER is going to wait..\n");
    wait();
    // wait();
    // printf(1, "FATHER woke up!\n");

    // exit();
}

void
test3(void){

    char *argv[0];
    signal(11, blaTest);

    sleep(400);
    exec("ls", argv);
    sleep(400);
}

void
test4(void){

    printf(1, "killRet: %d\n", signal(5, blaTest));
    printf(1, "killRet: %d\n", signal(66, blaTest));
    printf(1, "killRet: %d\n", signal(5, blaTest));
    printf(1, "killRet: %d\n", signal(17, blaTest));
}

void
blaTest(int i){
    printf(1, "************Insaide blaTest************\n");
}

int
main(void){

    // test1();
    // test2();
    // test3();
    test4();

    exit();
}