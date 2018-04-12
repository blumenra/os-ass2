#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int procPidsByPrioprity[3][10];
int priorityOneIndex = 0;
int priorityTwoIndex = 0;
int priorityThreeIndex = 0;
int rtimes[3];
int wtimes[3];
int iotimes[3];

void printAveragesOfPriority(int, int, int, int, int);
int IOfunc(void);
int simpleCalc(void);
void sanityTest(int type, int num_of_procs, int loop_size);
int getPriority(int);

int getPriority(int pid){

  for(int i=0; i < 3; i++){
    for(int j=0; j < 10; j++){
      if(procPidsByPrioprity[i][j] == pid)
        return i+1;
    }
  }

  return 0;
}

void sanityTest(int type, int num_of_procs, int loop_size){
    
  printf(1, "Starting sanityTest..\n");


  int wtime = 0, rtime = 0, iotime = 0;
  int wtimeAcc = 0, rtimeAcc = 0, iotimeAcc = 0;

  int procPids[num_of_procs];

  
  
  //Create lot's of sub procs
  int currPid;
  int i;
  for(i=0; i < num_of_procs; i++){

    currPid = fork();

    if(currPid == 0){ // I'M THE CHILD HERE..

      set_priority((i%3)+1);

      int j;
      for(j=0; j < loop_size; j++){

        // (*func)();
        int calcAcc = 0;

        if((type == 1) || (type == 2))
          calcAcc++;
        else{

          int fd = open("ls", 0);
          close(fd);
        }
      }

      exit(); // kill child
    }

    int index = i%3;
    switch(index){
      case 0:

        procPidsByPrioprity[index][priorityOneIndex++] = currPid;
        break;
      case 1:
      
        procPidsByPrioprity[index][priorityTwoIndex++] = currPid;
        break;
      case 2:
      
        procPidsByPrioprity[index][priorityThreeIndex++] = currPid;
        break;
      default:
          ;
    }

    procPids[i] = currPid;
  }

  // I'M THE FATHER HERE..


  //************************************ remove me
  // for(int i=0; i < priorityOneIndex; i++){
  //   printf(1, "pid_%d: %d\n", i, procPidsByPrioprity[0][i]);
  // }

  // for(int i=0; i < priorityTwoIndex; i++){
  //   printf(1, "pid_%d: %d\n", i, procPidsByPrioprity[1][i]);
  // }

  // for(int i=0; i < priorityThreeIndex; i++){
  //   printf(1, "pid_%d: %d\n", i, procPidsByPrioprity[2][i]);
  // }


  int k;
  for(k=0; k < num_of_procs; k++){

    int pid = procPids[k];
    wait2(pid, &wtime, &rtime, &iotime);

    switch(getPriority(pid)){

      case 1:
          rtimes[0] += rtime;
          wtimes[0] += wtime;
          iotimes[0] += iotime;
          break;
      case 2:
          rtimes[1] += rtime;
          wtimes[1] += wtime;
          iotimes[1] += iotime;
          break;
      case 3:
          rtimes[2] += rtime;
          wtimes[2] += wtime;
          iotimes[2] += iotime;
          break;
      default:
          ;
    }

    wtime = 0;
    rtime = 0;
    iotime = 0;
  }
  //************************************ remove me

  // int k;
  // for(k=0; k < num_of_procs; k++){

  //   wait2(procPids[k], &wtime, &rtime, &iotime);

  //   wtimeAcc += wtime;
  //   rtimeAcc += rtime;
  //   iotimeAcc += iotime;

  //   wtime = 0;
  //   rtime = 0;
  //   iotime = 0;
  // }

  printAveragesOfPriority(type, num_of_procs, wtimeAcc, rtimeAcc, iotimeAcc);
}

void printAveragesOfPriority(int type, int num_of_procs, int wtimeAcc, int rtimeAcc, int iotimeAcc){

  printf(1, "%d: wtime - %d, rtime - %d, iotime - %d (priority 1)\n",
          type,
          wtimes[0]/10,
          rtimes[0]/10,
          iotimes[0]/10);

  printf(1, "%d: wtime - %d, rtime - %d, iotime - %d (priority 2)\n",
          type,
          wtimes[1]/10,
          rtimes[1]/10,
          iotimes[1]/10);

  printf(1, "%d: wtime - %d, rtime - %d, iotime - %d (priority 3)\n",
          type,
          wtimes[2]/10,
          rtimes[2]/10,
          iotimes[2]/10);

}

int
main(int argc, char *argv[]){
  
  int num_of_procs = 10;



  sanityTest(1, num_of_procs, 10000000);     // simple calculation with a medium sized loop1
  sanityTest(2, num_of_procs, 1000000000);   // simple calculation with a very large loop
  sanityTest(3, num_of_procs, 1000);         // I/O operation with a medium sized loop
  sanityTest(4, num_of_procs, 10000);        // I/O operation with a very large loop

  exit();
}
