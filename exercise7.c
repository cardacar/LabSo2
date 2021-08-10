#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {

  int frk = fork();
  struct timeval timeC;
  int timeI;
  int timeE;
  
  if (frk < 0) {
    fprintf(stderr, "fork failed\n");
    exit(1);
  } else if (frk == 0) {
    gettimeofday(&timeC, NULL);
    timeI = timeC.tv_usec;
    char *myargs[argc];
    for (int i=1; i<argc; i++){
        myargs[i-1] = strdup(argv[i]);
    }
    myargs[argc-1] = NULL;
    execvp(myargs[0], myargs);  // run
  } else {
    wait(NULL);
    gettimeofday(&timeC, NULL);
    timeE = timeC.tv_usec;
    printf("Tiempo: %d", timeE-timeI);
  }
  return 0;
}