#include "types.h"
#include "stat.h"
#include "user.h"

#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

void customHandler(int i);
void printTestTitle(int testNum);
void miniTestForTest6(int testNum, int pid, int signum, int correctAns);
void miniTestForTest4(int testNum, int signum, void (*handler)(int), int correctAns);

// typedef void (*sighandler_t)(int);


/*
* checks if SIGSTOP stops the target and SIGCONT continues it
*/
void test0(void);

/*
* checks custom handler submition
*/
void test1(void);

/*
* checks that child ignores SIGCONT if it was sent while he was asleep
*/
void test2(void);


/*
* checks signal() return values correctness
*/
void test4(void);

/*
* checks that SIGKILL works
*/
void test5(void);

/*
* checks kill() return values correctness
*/
void test6(void);

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


void test0(void)
{
    printTestTitle(0);

    int pid= fork();
    if (pid==0){
        
        printf(1, "CHILD: starting..\n");
        int j;
        for(j=0 ; j < 50/*LARGE*/ ; j++)
        {
            printf(1,"*");
            int k=0;
            while(k < 5000000)
                k++;
        }

        printf(1, "\n");
        printf(1,"CHILD: exiting..\n");
        exit();
    }

    sleep(150);
    printf(1, "\n");
    printf(1, "FATHER: stopping child..\n");
    kill(pid,SIGSTOP);
    printf(1, "FATHER: going to sleeping for a while..\n");
    sleep(500);
    printf(1, "FATHER: continuing child..\n");
    kill(pid,SIGCONT);

    wait();
}

void
test1(void)
{
    printTestTitle(1);

    int signum = 10;

    int sonPid = fork();
    if(sonPid == 0){

        //son running
        printf(1, "SON: setting signal %d to custom function..\n", signum);
        signal(signum, customHandler);
        printf(1, "SON: going to sleep. when i'll wake up i'll execute the custom function..\n");
        sleep(500);

        printf(1, "SON: exiting..\n");
        exit();
    }

    //father running
    sleep(100);
    printf(1, "FATHER: sending signal %d to child..\n", signum);
    kill(sonPid, signum);

    wait();
}

void
test2(void)
{
    printTestTitle(2);

    int pid= fork();
    if (pid==0){
        
        printf(1, "CHILD: going to sleep..\n");
        sleep(300);
        printf(1,"CHILD: woke up!\n");

        exit();
    }

    sleep(150);
    printf(1, "\n");
    printf(1, "FATHER: stopping child..\n");
    kill(pid,SIGSTOP);
    printf(1, "FATHER: going to sleeping for a while..\n");
    sleep(500);

    wait();
}


void
test4(void){
    printTestTitle(4);

    miniTestForTest4(1, 5, customHandler, -1); //correct ans = -1 (SIG_DEF)
    miniTestForTest4(2, 66, customHandler, -2);
    miniTestForTest4(3, 5, customHandler, (int)customHandler);
    miniTestForTest4(4, SIGCONT, customHandler, SIGCONT);
    miniTestForTest4(5, SIGCONT, customHandler, (int)customHandler);
    miniTestForTest4(6, SIGKILL, customHandler, SIGKILL);
    miniTestForTest4(6, SIGKILL, customHandler, (int)customHandler);
}

/*
* father let's son print stars and kills him before finishing priting all stars 
* and printing the line 'CHILD: exiting..'
*/
void
test5(void){
    printTestTitle(5);
    
    int sonPid= fork();
    if (sonPid==0){
        
        printf(1, "CHILD: starting..\n");
        int j;
        for(j=0 ; j < 50/*LARGE*/ ; j++)
        {
            printf(1,"*");
            int k=0;
            while(k < 5000000)
                k++;
        }

        printf(1, "\n");
        printf(1,"CHILD: exiting..\n");
        exit();
    }

    sleep(100);
    kill(sonPid,SIGKILL);

    wait();
    printf(1, "\nFATHER is exiting after killing son..\n");
}
    

void
test6(void){
    printTestTitle(6);

    int sonPid = fork();
    if(sonPid == 0){

        sleep(400);
        printf(1, "SON woke up!\n");

        exit();
    }

    sleep(100);


    miniTestForTest6(1, sonPid, 31, 0);
    miniTestForTest6(2, sonPid, 32, -1);
    miniTestForTest6(3, 666, 0, -1);
    miniTestForTest6(4, 666, -1, -1);
    miniTestForTest6(5, sonPid, SIGSTOP, -1);

    wait();
}

void
customHandler(int i){
    printf(1, "************Inside customHandler************\n");
    printf(1, "signal %d\n", i);
    printf(1, "************Inside customHandler************\n");
}

void
printTestTitle(int testNum){
    
    printf(1, "***TEST %d***\n", testNum);
}

void
miniTestForTest6(int testNum, int pid, int signum, int correctAns){
    
    printf(1, "%d: kill(%d, %d): ",testNum ,pid ,signum);
    int ret = kill(pid, signum); // valid pid, Invalid signal
    if(ret == correctAns)
        printf(1, "PASSED!\n");
    else
        printf(1, "FAIL: expected %d and got %d!\n", correctAns, ret);
}

void
miniTestForTest4(int testNum, int signum, void (*handler)(int), int correctAns){
    
    printf(1, "%d: signal(%d, handler): ",testNum ,signum);
    int ret = (int)signal(signum, handler); // valid pid, Invalid signal
    if(ret == correctAns)
        printf(1, "PASSED!\n");
    else
        printf(1, "FAIL: expected %d and got %d!\n", correctAns, ret);
}

int
main(void){

    test0();
    test1();
    test2();
    test5();
    test6();
    test4();

    exit();
}