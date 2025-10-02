#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define NUM_CHILDREN 5
#define MAX_MSG 64
#define HEADER(index, len) (((index & 0xFFFF) << 16) | (len & 0xFFFF))
#define GET_LENGTH(hdr) ((hdr) & 0xFFFF)
#define GET_INDEX(hdr) (((hdr) >> 16) & 0xFFFF)

int build_msg(char *msg, int index) {
    int len = 0;
    const char *prefix = "Hello from child ";
    int prefix_leng = strlen(prefix);
    memcpy(msg, prefix, prefix_leng);
    len = prefix_leng;
    if (index >= 10) {
        msg[len++] = '0' + (index / 10);
        msg[len++] = '0' + (index % 10);
    } else {
        msg[len++] = '0' + index;
    }

    msg[len] = '\0';
    return len;
}

//uint32 for 4-byte headers
//uint64 for memory addresses
void child_log(int index, char *shared) {
    char msg[MAX_MSG];
    char *end_msg = shared + PGSIZE;
    for (int m = 0; m < 40; m++) { //to test buffer overflow
        int len = build_msg(msg, index);
        char *ptr = shared;
        int wrote = 0;
        while (ptr + 4 + len <= end_msg) {
            // if ((uint64)ptr + 4 + len > (uint64)end_msg)
            //     break;
            uint32 *hdr = (uint32 *)ptr;
            if (*hdr == 0) {
                uint32 new_hdr = HEADER(index, len);
                if (__sync_val_compare_and_swap(hdr, 0, new_hdr) == 0) {
                    memcpy(ptr + 4, msg, len);
                    wrote = 1;
                    break;
                }
            }
            uint32 exist = GET_LENGTH(*hdr);
            ptr += 4 + exist;
            ptr = (char *)(((uint64)ptr + 3) & ~3);
        }
        if (!wrote) {
            ptr = shared;
            while (ptr + 4 <= end_msg) {
                if ((uint64)ptr + 4 > (uint64)end_msg)
                    break;

                uint32 *hdr = (uint32 *)ptr;
                if (__sync_val_compare_and_swap(hdr, 0, HEADER(index, 0)) == 0)
                    break;

                uint32 exist = GET_LENGTH(*hdr);
                ptr += 4 + exist;
                ptr = (char *)(((uint64)ptr + 3) & ~3);
            }
            break;
        }
      // Delay for process interleaving 
      //if we want it to be sorted just delete this line )
      for (volatile int d = 0; d < (index + m) * 50000; d++);
    }
}

void read_logs(char *shared) {
    char *ptr = shared;
    char *end_msg = shared + PGSIZE;
    int counter = 0;

    while (ptr + 4 <= end_msg) {
      uint32 *hdr = (uint32 *)ptr;
      uint32 h = *hdr;
      if (h == 0)
         break;

      int child = GET_INDEX(h);
      int len = GET_LENGTH(h);

      if (ptr + 4 + len > end_msg) {
          printf("Corrupt entry at %p, stopping\n", ptr);
            break;
      }

      if (len == 0) {
          printf("Child %d: FAILED (buffer full)\n", child);
          ptr += 4;
      } else {
          char buf[MAX_MSG];
          memcpy(buf, ptr + 4, len);
          buf[len] = '\0';
          printf("[child %d] %s\n", child, buf);
          ptr += 4 + len;
      }
      ptr = (char *)(((uint64)ptr + 3) & ~3);
      counter++;
  }

  printf("Total messages read: %d\n", counter);
}

int main() {
    printf("Starting multi-process logging with %d children\n", NUM_CHILDREN);
    int parent = getpid();
    char *shared = malloc(PGSIZE);
    if (!shared) {
        printf("malloc failed\n");
        exit(1);
    }
    memset(shared, 0, PGSIZE);
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (fork() == 0) {
            uint64 mapped = map_shared_pages(parent, getpid(), (uint64)shared, PGSIZE);
            if (mapped == 0 || mapped == (uint64)-1) {
              //  printf("Child %d: map failed\n", i);
                exit(1); //silent exit
            }
            child_log(i, (char *)mapped);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++) wait(0);

    printf("\n--- Reading logs ---\n");
    read_logs(shared);
    printf("--- The End ---\n");

    exit(0);
}
