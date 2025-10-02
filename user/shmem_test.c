#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid;
  char *shared_data;
  uint64 mapped_addr;
  int disable_unmap = 0;
  int pipe_fd[2];
  int parent_pid;
  uint64 shared_addr;
  
  if(argc > 1 && strcmp(argv[1], "no_unmap") == 0) {
    disable_unmap = 1;
  }
  //create pipe for communication
  if(pipe(pipe_fd) < 0) {
    printf("pipe failed\n");
    exit(1);
  }
  parent_pid = getpid();
  printf("Parent process starting (PID: %d)\n", parent_pid);
  //allocate some memory in parent
  shared_data = malloc(64);
  if(shared_data == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  strcpy(shared_data, "Hello world");
  printf("Parent allocated memory at %p, wrote: %s\n", shared_data, shared_data);
  pid = fork();
  if(pid < 0) {
    printf("fork failed\n");
    exit(1);
  }
  if(pid == 0) {
    //child process
    close(pipe_fd[1]); //close write end
    //read parent info from pipe
    if(read(pipe_fd[0], &parent_pid, sizeof(parent_pid)) != sizeof(parent_pid) ||
       read(pipe_fd[0], &shared_addr, sizeof(shared_addr)) != sizeof(shared_addr)) {
      printf("Child: failed to read from pipe\n");
      exit(1);
    }
    close(pipe_fd[0]);
    printf("Child process started (PID: %d), parent PID: %d\n", getpid(), parent_pid);
    printf("Child size before mapping: %d\n", sbrk(0));
    //map shared memory from parent to child
    mapped_addr = map_shared_pages(parent_pid /*source PID */, 
                                   getpid() /*destination PID */, 
                                   shared_addr, 64);
    if(mapped_addr == (uint64)-1) {
      printf("map_shared_pages failed\n");
      exit(1);
    }
    printf("Child size after mapping: %d\n", sbrk(0));
    printf("Child mapped memory at %p\n", (void*)mapped_addr);
    //write message for parent
    strcpy((char*)mapped_addr, "Hello daddy");
    printf("Child wrote: %s\n", (char*)mapped_addr);
    if(!disable_unmap) {
      //unmap shared memory
      if(unmap_shared_pages(mapped_addr, 64) != 0) {
        printf("unmap_shared_pages failed\n");
        exit(1);
      }
      printf("Child size after unmapping: %d\n", sbrk(0));
      //test malloc after unmapping
      char *test_malloc = malloc(32);
      if(test_malloc == 0) {
        printf("malloc after unmap failed\n");
        exit(1);
      }
      printf("Child size after malloc: %d\n", sbrk(0));
      printf("Child malloc succeeded at %p\n", test_malloc);
      free(test_malloc);
    }
    printf("Child exiting\n");
    exit(0);
  } else {
    //parent process
    close(pipe_fd[0]); //close read end
    //send parent info to child
    shared_addr = (uint64)shared_data;
    if(write(pipe_fd[1], &parent_pid, sizeof(parent_pid)) != sizeof(parent_pid) ||
       write(pipe_fd[1], &shared_addr, sizeof(shared_addr)) != sizeof(shared_addr)) {
      printf("Parent: failed to write to pipe\n");
      exit(1);
    }
    close(pipe_fd[1]);
    wait(0); //wait for child to complete
    printf("Parent reading shared data: %s\n", shared_data);
    if(!disable_unmap) {
      printf("Test completed successfully\n");
    } else {
      printf("Test completed (child exited without unmapping)\n");
    }
    free(shared_data);
  }
  
  return 0;
}