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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>

/*** defines ***/
#define VERSION "0.0.1"
#define TAB_S 8
#define QUIT_UNSAVED 1
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRING (1<<1)
//arrow keys
// A stands for Arrow
#define A_UP 1000
#define A_DOWN 1001
#define A_LEFT 1002
#define A_RIGHT 1003

/*** data ***/

struct EditorSyntax {
    char *filetype;
    char **filematch;
    int flags;
};

enum EditorKeys {
    BACKSPACE = 8,
    DEL = 127,
    INSERT = 'i',
    UP = 'k', 
    DOWN = 'j',
    LEFT = 'l',
    RIGHT = 'h',
    H_KEY = 'a',
    E_KEY = 'd',
    P_UP = 'n',
    P_DOWN = 'm',
    ESC = 27
};

enum EditorHighlights {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

typedef struct {
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *hl;
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
    int dirty;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct EditorSyntax *syntax;
    struct termios origin;
};

// using the editorconfig
struct EditorConfig E;

/*** filetypes ***/

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };

struct EditorSyntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRING
  },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

// prototypes
char *EditorPrompt(char *prompt, void (*callback)(char *, int));


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

int EditorReadInsertKey() {
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
                    case 'A': return A_UP;
                    case 'B': return A_DOWN;
                    case 'C': return A_LEFT;
                    case 'D': return A_RIGHT;
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

/*** syntax highlighting ***/

int is_separator(int c) {
    return isspace(c) || c == '\0'  || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void EditorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (E.syntax == NULL) return;

    int prev_sep = 1;
    int in_string = 0;

    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
        
        if (E.syntax->flags & HL_HIGHLIGHT_STRING) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else { 
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }


        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
            (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
            prev_sep = is_separator(c);
            i++;
        }
  }
}

int EditorSyntaxToColor(int hl) {
    switch (hl) {
        case HL_COMMENT: return 31;
        case HL_STRING: return 33;
        case HL_NUMBER: return 32;
        case HL_MATCH: return 34;
        default: return 37;
    }
}

void EditorSelectSyntaxHighlight() {
    E.syntax = NULL;
    if (E.filename == NULL) return;

    char *ext = strrchr(E.filename, '.');

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct EditorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i]) {
            int is_ext = (s->filematch[i][0] == '.');

            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
               (!is_ext && strstr(E.filename, s->filematch[i]))) {
                E.syntax = s;

                int filerow;
                for (filerow = 0; filerow < E.numrows; filerow++) {
                    EditorUpdateSyntax(&E.row[filerow]);
                }

                return;
            }

            i++;
        }
    }
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

int EditorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int Cx;
  for (Cx = 0; Cx < row->size; Cx++) {
    if (row->chars[Cx] == '\t')
      cur_rx += (TAB_S - 1) - (cur_rx % TAB_S);
    cur_rx++;
    if (cur_rx > rx) return Cx;
  }
  return Cx;
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

    EditorUpdateSyntax(row);
}


void EditorAppendRows(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    EditorUpdateRows(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void EditorRowInsert(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    EditorUpdateRows(row);
    E.dirty++;
}

void EditorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1); 
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size += len] = '\0';
    EditorUpdateRows(row);
    E.dirty++;
}

void EditorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void EditorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    EditorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}

    
void EditorRowDelChar(erow *row, int at) {
    if (at >= row->size || at < 0) return; 

    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    row->chars = realloc(row->chars, row->size + 1);
    if (row->chars == NULL) exit(1); 
    row->chars[row->size] = '\0';
}

/*** editor operations ***/

void EditorInsertChar(int c) {
    if (E.Cy == E.numrows) {
        EditorAppendRows(E.numrows, " ", 0);
    }
    EditorRowInsert(&E.row[E.Cy], E.Cx, c);
    E.Cx++; 
}

void EditorInsertNewLine() {
    if (E.Cx == 0) {
        EditorAppendRows(E.Cy, "", 0);
    } else {
        erow *row = &E.row[E.Cy];
        EditorAppendRows(E.Cy + 1, &row->chars[E.Cx], row->size - E.Cx);
        row = &E.row[E.Cy];
        row->size = E.Cx;
        row->chars[row->size] = '\0';
        EditorUpdateRows(row);
    }
    E.Cy++;
    E.Cx = 0;
}

void EditorDelChar() {
    if (E.Cy == E.numrows && E.Cx == 0) return;

    if (E.Cx == 0 && E.Cy > 0) {
        erow *prev_row = &E.row[E.Cy - 1];
        erow *current_row = &E.row[E.Cy];

        prev_row->chars = realloc(prev_row->chars, prev_row->size + current_row->size + 1);
        if (prev_row->chars == NULL) exit(1); 
        memcpy(&prev_row->chars[prev_row->size], current_row->chars, current_row->size + 1);
        prev_row->size += current_row->size;

        free(current_row->chars);
        memmove(&E.row[E.Cy - 1], &E.row[E.Cy], sizeof(erow) * (E.numrows - E.Cy));
        E.numrows--;
        E.Cy--;
        E.Cx = prev_row->size; 
    } else {
        EditorRowDelChar(&E.row[E.Cy], E.Cx - 1);
        E.Cx--;
    }

    E.dirty++;
}




// Output Message


void EditorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}



/*** file i/o ***/

char *EditorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++) {
        totlen += E.row[j].size + 1;
    }
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}


void EditorOpen(char* filename) {
    free(E.filename);
    E.filename = strdup(filename);

    EditorSelectSyntaxHighlight();

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
            EditorAppendRows(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
} 

void EditorSave() {
    if (E.filename == NULL) {
        E.filename = EditorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            EditorSetStatusMessage("Save aborted");
            return;
        }
        EditorSelectSyntaxHighlight();
    }

    int len;
    char *buf = EditorRowsToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                EditorSetStatusMessage("%d bytes written to disk | Save succsesful", len);
                return;
            }
        }
        close(fd);
    }


    free(buf);
    EditorSetStatusMessage("Error encountered while saving | Error: %s", strerror(errno));
}

/*** search ***/

void EditorFindCallback(char *query, int key) {
    static int last_match = -1;
    static int direction = 1;


    static int saved_hl_line;
    static char *saved_hl = NULL;
    if (saved_hl) {
        memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == A_UP || key == A_DOWN) {
        direction = 1;
    } else if (key == A_LEFT || key == A_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    } 

    if (last_match == -1) direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < E.numrows; i++) {
        current += direction;
        if (current == -1) current =  E.numrows - 1;
        else if (current == E.numrows) current = 0;
        
        erow  *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match) {
           last_match = current;
           E.Cy = current;
           E.Cx = EditorRowRxToCx(row, match - row->render);
           E.rowoff = E.numrows;

           saved_hl_line = current;
           saved_hl = malloc(row->rsize);
           memcpy(saved_hl, row->hl, row->rsize);
           memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
           break;
        }
    }
}

void EditorFind() {
    // old positions
    int old_Cx = E.Cx;
    int old_Cy = E.Cy;
    int old_coloff = E.coloff;
    int old_rowoff = E.rowoff;

    char *query = EditorPrompt("Search: %s (esc to cancel)", EditorFindCallback);
    
    if (query) {
        free(query);
    } else {
        E.Cx = old_Cx;
        E.Cy = old_Cy;
        E.coloff = old_coloff;
        E.rowoff = old_rowoff;
    }
}

/*** output ***/



void EditorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.S_cols) msglen = E.S_cols;
    if (msglen && time(NULL) - E.statusmsg_time < 5) {
        abAppend(ab, E.statusmsg, msglen);
    }
}

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
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.S_cols) len = E.S_cols;
            char *pC = &E.row[filerow].render[E.coloff];
            unsigned char *hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            int j;
            for (j = 0; j < len; j++) {
                if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abAppend(ab, "\x1b[39m", 5);
                        current_color = -1;
                    }
                    abAppend(ab, &pC[j], 1);
                } else {
                    int color = EditorSyntaxToColor(hl[j]);
                    if (current_color == -1) {
                        current_color = color;
                        char buff[16];
                        int clen = snprintf(buff, sizeof(buff),  "\x1b[%dm", color);
                        abAppend(ab, buff, clen);
                    }
                    abAppend(ab, &pC[j], 1);
                }
            }
            if (current_color != -1) {
                abAppend(ab, "\x1b[39m", 5);
            }
        }
        
        // End the line
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}


void EditorDrawStatusLine(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
         E.filename ? E.filename : "[No Name]", E.numrows,
         E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
        E.syntax ? E.syntax->filetype : "no ft", E.Cy + 1, E.numrows);
    if (len > E.S_cols) len = E.S_cols;
    abAppend(ab, status, len);
    while (len < E.S_cols) {
        if (E.S_cols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void EditorRefreshScreen() {
    EditorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab,  "\x1b[H", 3);

    EditorDrawRows(&ab);
    EditorDrawStatusLine(&ab);
    EditorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.Cy - E.rowoff) + 1,
                                              (E.Rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}


/*** input ***/

char *EditorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    EditorSetStatusMessage(prompt, buf);
    EditorRefreshScreen();
    int c = EditorReadKey();
    if (c == DEL || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      EditorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        EditorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback) callback(buf, c);
  }
}


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

void InsertMoveCursor(int key) {
    erow *row = (E.Cy >= E.numrows) ?  NULL : &E.row[E.Cy];

    switch (key){
     case A_RIGHT:
            if (E.Cx != 0) {
                E.Cx--;
            } else if (E.Cy > 0) {
                E.Cy--;
                E.Cx = E.row[E.Cy].size;
            }

            break;
        case A_LEFT:
            if (row && E.Cx < row->size) {
                E.Cx++;
            } else if (row && E.Cx == row->size) {
                E.Cy++;
                E.Cx = 0;
            }
            break;
        case A_UP:
            if (E.Cy != 0) {
                E.Cy--;
            }
            break;
        case A_DOWN:
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

    
    /*** Insert mode ***/

void InsertMode(int c) {
    EditorSetStatusMessage("-- INSERT MODE --");

    switch (c) {
        default:
            EditorInsertChar(c);
            break;


        case A_UP:
        case A_DOWN:
        case A_RIGHT:
        case A_LEFT:
            InsertMoveCursor(c);
            break;

        case '\r':
            EditorInsertNewLine();
            break;
            
        case BACKSPACE:
            EditorDelChar();
            EditorRefreshScreen();
            break;
        
        case ESC:
            break;

        case DEL:
            EditorMoveCursor(RIGHT);
            EditorDelChar();
            break;
    
    }
    
}


void EditorProcessKeypress() {
    static int quit_times = QUIT_UNSAVED;

    char c = EditorReadKey();
    char i = c;
    
    switch (c) {

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                EditorSetStatusMessage("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q %d more time to quit.", quit_times);
                quit_times--;
                return;
            }

            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);

            exit(0);
            break;
        
        // save files
        case CTRL_KEY('s'):
            EditorSave();
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



        case '/':
            EditorFind();
            break;


        //  basic movement 
        case UP:
        case DOWN:
        case RIGHT:
        case LEFT:
          EditorMoveCursor(c);
          break;

        case CTRL_KEY('l'):
          break;


        case INSERT:
            while (i != ESC) {  
                i = EditorReadInsertKey();
                InsertMode(i);
                EditorRefreshScreen();
            }
            EditorSetStatusMessage("-- Normal Mode --");
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
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.syntax = NULL;

    if (GetWS(&E.S_rows , &E.S_cols) == -1) errors("GetWS");
    E.S_rows -= 2;
}

int main(int argc, char* argv[]) {
      EnableRawMode();
      InitEditor();
      if (argc >= 2) {
          EditorOpen(argv[1]);
      }

     EditorSetStatusMessage("Move around with h, j, k, l | Exit with Ctrl + Q | Save with Ctrl S");

      while (1) {
          EditorRefreshScreen();
          EditorProcessKeypress();
      }

      return 0;
}

// I am truly sorry for you having to read and see all of this code 
