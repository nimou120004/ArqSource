#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/poll.h>
#include "mtpm.h"

int getkey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}

mtpm mtpm1;

int main(int argc, char *argv[])
{
    mtpm1.init_mtpm(argc, argv);
    struct pollfd ufds[1];
    ufds[0].fd = mtpm1.s;
    ufds[0].events = POLLIN; // check for just normal data
    int rv;
    int key;
    for (;;)
    {
        rv = poll(ufds, 1, mtpm1.timeout); //wait for events on socket s for 1 millisecond timeout
        if (rv == -1) {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) {
            //nothing here
        } else {
            // check for events on s1:
            if (ufds[0].revents & POLLIN) {
                mtpm1.check_udp_socket();
                mtpm1.check_pbb();
            }
        }
        key = getkey();
        /* terminate loop on ESC (0x1B) or Ctrl-D (0x04) on STDIN */
        if (key == 0x1B || key == 0x04)
            break;
    }
    return 0;
}
