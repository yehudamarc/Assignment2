#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  if(argc < 2){
    printf(2, "usage: kill pid...\n");
    exit();
  }

  kill(atoi(argv[2]), atoi(argv[2]));
  exit();
}
