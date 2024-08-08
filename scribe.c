/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>

/*** defines ***/
#define VERSION "0.0.1"
#define TAB_S 8
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

/*** data ***/

enum EditorKeys {
    UP = 'k', 
    DOWN = 'j',
    LEFT = 'l',
    RIGHT = 'h',
    H_KEY = 'a',
    E_KEY = 'd',
    P_UP = 'n',
    P_DOWN = 'm'
};

typedef struct {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

struct EditorConfig {
    int Cx, Cy; // cursor position Cx: Cursos horizontal position Cy: Cursor vertical position
    int Rx;
    int rowoff;
    int coloff;
    int S_rows;
    int S_cols;
    int numrows;
    erow *row;
    struct termios origin;
};

// using the editorconfig
struct EditorConfig E;

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
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.origin) == -1) {
        errors("tcsetattr");
    }
}

// enable raw mode (disabling echo)
void EnableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.origin) == -1) errors("tcgetattr");
    atexit(DisableRawMode); // disable raw mode at end

    struct termios raw = E.origin;
   
    // disableing stuff
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN| ISIG); 

    raw.c_cc[VMIN] = 0;
    

    raw.c_cc[VTIME] = 1;

     if (tcsetattr(STDIN_FILENO, TCSAFLUSH,&raw) == -1) errors("tcsetattr");
}

int EditorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) errors("read");
    }
    
    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return H_KEY;
                        case '4': return E_KEY;
                        case '5': return P_UP;
                        case '6': return P_DOWN;
                        case '7': return H_KEY;
                        case '8': return E_KEY;
                    }
                }

            } else {
                switch (seq[1]) {
                    case 'A': return UP;
                    case 'B': return DOWN;
                    case 'C': return LEFT;
                    case 'D': return RIGHT;
                    case 'H': return H_KEY;
                    case 'F': return E_KEY;
            }

        }
       } else if (seq[0] == 'O') {
           switch (seq[1]) {
                case 'H': return H_KEY;
                case 'F': return E_KEY;
           }
       }

        return '\x1b';

    } else {
        return c;
    }
}

int GetCP(int *rows, int *cols) {
    // decleration
    char buff[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    
    while (i == sizeof(buff) -1) {
        if (read(STDIN_FILENO, &buff[i], 1) != 1) break;
        if (buff[i] == 'R') break;
        i++;
    }
    buff[i] = '\0';

    if (buff[0] != '\x1b' || buff[1] != '[') return -1;
    if (sscanf(&buff[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

int GetWS(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return GetCP(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    }
    return 0;
}


// append buffer
struct abuf {
    char *b;
    int len;
};

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** row operations ***/

int EditorRowCxToRx(erow *row, int Cx) {
  int Rx = 0;
  int j;
  for (j = 0; j < Cx; j++) {
    if (row->chars[j] == '\t')
      Rx += (TAB_S - 1) - (Rx % TAB_S);
    Rx++;
  }
  return Rx;
}

void EditorUpdateRows(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }

    free(row->render);
    row->render = malloc(row->size + tabs*(TAB_S - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_S != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;
}

void EditorAppendRows(char *s, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    EditorUpdateRows(&E.row[at]);

    E.numrows++;
}

/*** file i/o ***/

void EditorOpen(char* filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) errors("fopen");

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
            while (linelen > 0 && (line[linelen - 1] == '\n' ||
                        line[linelen - 1] == '\r')) {
                linelen--;
            }
            EditorAppendRows(line, linelen);
    }
    free(line);
    fclose(fp);
} 

/*** output ***/

void EditorScroll() {
    E.Rx = 0;
    if (E.Cy < E.numrows) {
        E.Rx = EditorRowCxToRx(&E.row[E.Cy], E.Cx);
    }

    if (E.Cy < E.rowoff) {
        E.rowoff = E.Cy;
    }
    if (E.Cy >= E.rowoff + E.S_rows) {
        E.rowoff = E.Cy - E.S_rows + 1;
    }
    if (E.Rx < E.coloff) {
        E.coloff = E.Rx;
    }
    if (E.Rx >= E.coloff + E.S_cols) {
        E.coloff = E.Rx - E.S_cols + 1;
    }
}

// Tildes
void EditorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.S_rows; y++) {
        int filerow = y + E.rowoff;

    if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.S_rows / 3) {
                char welcome[100];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                        "Scribe -- version %s", VERSION);
                if (welcomelen > E.S_cols) welcomelen = E.S_cols;
                int padding = (E.S_cols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
           
            // print out the messages
            abAppend(ab, welcome, welcomelen);

        } else {
            abAppend(ab, "~", 1);
        }
    } else {
        int len = E.row[filerow].rsize - E.coloff;
        if (len < 0) len = 0;
        if (len > E.S_cols) len = E.S_cols;
        abAppend(ab, &E.row[filerow].render[E.coloff] , len);
    }
        
    // drawing rows
    abAppend(ab, "\x1b[K", 3);
    if (y < E.S_rows - 1) {
      abAppend(ab, "\r\n", 2);
    }

    }
}

void EditorRefreshScreen() {
    EditorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab,  "\x1b[H", 3);

    EditorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.Cy - E.rowoff) + 1,
                                              (E.Rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** input ***/
void EditorMoveCursor(char key) {
    erow *row = (E.Cy >= E.numrows) ?  NULL : &E.row[E.Cy];

    switch (key) {
        case RIGHT:
            if (E.Cx != 0) {
                E.Cx--;
            } else if (E.Cy > 0) {
                E.Cy--;
                E.Cx = E.row[E.Cy].size;
            }

            break;
        case LEFT:
            if (row && E.Cx < row->size) {
                E.Cx++;
            } else if (row && E.Cx == row->size) {
                E.Cy++;
                E.Cx = 0;
            }
            break;
        case UP:
            if (E.Cy != 0) {
                E.Cy--;
            }
            break;
        case DOWN:
            if (E.Cy < E.numrows) {
                E.Cy++;
            }
            break;

        row = (E.Cy >= E.numrows) ? NULL : &E.row[E.Cy];
        int rowlen = row ? row->size : 0;
        if (E.Cx > rowlen) {
            E.Cx = rowlen;
        }

    }
}

void EditorProcessKeypress() {
    char c = EditorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);

            exit(0);
            break;

        // Page keys 
        case P_UP:
        case P_DOWN:
            {
                if (c == P_UP) {
                    E.Cy = E.rowoff;
                } else if (c == P_DOWN) {
                    E.Cy = E.rowoff + E.S_rows - 1;
                    if (E.Cy > E.numrows) E.Cy = E.numrows;
                }

                int times = E.S_rows;
                while (times--) {
                    EditorMoveCursor(c == P_UP ? UP : DOWN);
                }
            break;
            }


        // Home and End Keys
        case H_KEY:
            E.Cx = 0;
            break;

        case E_KEY:
            if (E.Cy < E.numrows) {
                E.Cx = E.row[E.Cy].size;
            }
            break;

        //  basic movement 
        case UP:
        case DOWN:
        case RIGHT:
        case LEFT:
          EditorMoveCursor(c);
          break;
    }
}

/*** init ***/

void InitEditor() {
    E.Cx = 0;
    E.Cy = 0;
    E.Rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row =  NULL;

    if (GetWS(&E.S_rows , &E.S_cols) == -1) errors("GetWS");
}

int main(int argc, char* argv[]) {
      EnableRawMode();
      InitEditor();
      if (argc >= 2) {
          EditorOpen(argv[1]);
      }

      while (1) {
          EditorRefreshScreen();
          EditorProcessKeypress();
      }

      return 0;
}
