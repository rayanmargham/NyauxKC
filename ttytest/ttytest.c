#include <stdio.h>
#include <strings.h>
#include <termios.h>

static struct termios stored_settings;
int main(int argc, char **argv) {
    if(argc != 1) {
        printf("%s takes no arguments.\n", argv[0]);
        return 1;
    }
    tcgetattr(0,&stored_settings);

    struct termios new_settings = stored_settings;
    new_settings.c_lflag |= (ICANON | ECHO);
    tcsetattr(0,TCSANOW,&new_settings);
    while (1)
      ;;
    
    return 0;
}
