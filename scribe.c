/*** includes ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>


/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

// making a structure
struct termios origin;

/*** funcs ***/


     /*** terminal ***/

// void for error handaling (I think)
void errors(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

// disable raw mode at end
void DisableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origin) == -1) {
        errors("tcsetattr");
    }
}

// enable raw mode (disabling echo)
void EnableRawMode() {
    if (tcgetattr(STDIN_FILENO, &origin) == -1) errors("tcgetattr");
    atexit(DisableRawMode); // disable raw mode at end

    struct termios raw = origin;
   
    // disableing stuff
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN| ISIG); 

    raw.c_cc[VMIN] = 0;
    

    raw.c_cc[VTIME] = 1;

     if (tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw) == -1) errors("tcsetattr");
}

char EditorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) errors("read");
    }
    return c;
}

/*** output ***/
void EditorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n");
    }
}

void EditorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO,  "\x1b[H", 3);

    EditorDrawRows();
}

/*** input ***/
void EditorProcessKeypress() {
    char c = EditorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);

            exit(0);
            break;
    }
}

/*** init ***/
int main() {
      EnableRawMode();

      while (1) {
          EditorRefreshScreen();
          EditorProcessKeypress();
      }

      return 0;
}
