#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  //int i;

  if(argc < 2){
    printf(2, "usage: kill pid...\n");
    exit();
  }
  //for(i=1; i<argc; i++)
  // kill(atoi(argv[1]), 19); // for debugging
  kill(atoi(argv[1]), atoi(argv[2]));

  exit();
}
