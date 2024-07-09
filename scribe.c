// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

// making a structure
struct termios origin;

// disable raw mode at end
void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origin);
}

// enable raw mode (disabling echo)
void EnableRawMode() {
    tcgetattr(STDIN_FILENO, &origin);
    atexit(DisableRawMode); // disable raw mode at end

    struct termios raw = origin;
   
    // disableing stuff
    raw.c_iflag &= ~(IXON);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); 

    tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw);
}

int main() {
      EnableRawMode();

      char c;
      while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (iscntrl(c)) {
            printf("%d\n", c);
        }else {
            printf("%d (%c)\n", c, c);
        }
      };
      return 0;
}
