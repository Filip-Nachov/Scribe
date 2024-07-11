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
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN| ISIG); 

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw);
}

int main() {
      EnableRawMode();

      while (1){
        char c = '\0';
        read(STDIN_FILENO, &c, 1)

        if (iscntrl(c)) {
            printf("%d\n", c);
        }else {
            printf("%d (%c)\n", c, c);
        }

        if (c == 'q') break;
      };
      return 0;
}
