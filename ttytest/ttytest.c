#include <strings.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/utsname.h>
void other_fn() {
    printf("I am a child\n");
}
int main(int argc, char **argv) {

 printf("Hello, world!\n");

    for (int i = 0; i < 1024; i++) {
        int forked = fork();
        if (forked) {
            printf("I am parent for %i\n", forked);
        } else {
            other_fn();
            break;
        }
    }
  sched_yield(); 
    return 0;
}
