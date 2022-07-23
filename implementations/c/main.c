#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SUCCESS       0x0
#define BAD_CHAR      0x1
#define IO_ERROR      0x2
#define INCOMPLETE    0x4
#define EOF_REACHED   0x8
#define L_TOO_LONG   0x10
#define V_TOO_LONG   0x20
#define TOO_MANY     0x40
#define NO_MEM       0x80
#define NO_KEY       0x100
#define NOT_FOUND    0x200
#define NOT_PRESENT  0x400
#define ERROR_MASK   0x7FF

#define SCHEMA       0x1000
#define START_LINE   0x2000
#define VAL_CONT     0x4000
#define RECORD       0x8000
#define TOKEN_MASK   0xF000

#define MAX_LENGTH 65

const char *error_error(int error) {
    switch (error & ERROR_MASK) {
        case SUCCESS:
            return "successful";
        case BAD_CHAR:
            return "bad or missing character (misplaced or missing tab)";
        case IO_ERROR:
            return "I/O error";
        case INCOMPLETE:
            return "incomplete stanza (not enough lines)";
        case EOF_REACHED:
            return "EOF reached";
        case L_TOO_LONG:
            return "line was too long (must be 511 characters or less)";
        case NO_MEM:
            return "memory allocation error";
        case NO_KEY:
            return "no key field";
    }
    return NULL;
}

const char *error_token(int error) {
    switch (error & TOKEN_MASK) {
        case SCHEMA:
            return "schema record";
            break;
        case START_LINE:
            return "record start line";
            break;
        case VAL_CONT:
            return "value continuation";
            break;
        case RECORD:
            return "record";
            break;
    }
    return NULL;
}

void report_error(int error, const unsigned long *l) {
    fprintf(stderr, "ERROR: On line %lu, while parsing token %s: %s\n",
            *l,
            error_token(error),
            error_error(error));
}

// The maximum number of characters, INCLUDING NULL TERMINAL AND LINE
// DELIMITER, a line can be.
#define MAX_LINE_SIZE 512

// The maximum number of non-empty lines a stanza can have.
// A stanza is a bunch of lines that have new empty line in between them.
#define MAX_STANZA_SIZE 64

// Takes a simple lval and a maybe-complex rval


/* Takes a file, a pointer to an integer for recording the tab
 * position, and a pointer to an unsigned for recording the line number.
 * Records the position of the first tab character, if present.
 *
 */

typedef struct {
    char *line;
    size_t length;
    int tab_pos;
} stanza_line_t;


inline int finish_line(unsigned char **line_buffer, size_t spot, size_t *line_size) {
    (*line_buffer)[spot] = '\0';
    *line_size = spot+1;
}

/* Attempts to allocate a new pointer and copy the line into it.
 * line MUST be of size MAX_LINE_SIZE.
 * Do not call this method unless `expect_line` was first called.
 */
int create_from_line(stanza_line_t *sl, char **line_buffer, size_t line_size, int tab_pos) {
    sl->line = malloc(line_size);
    if (sl->line == NULL) {
        return NO_MEM;
    }
    memcpy(sl->line, (*line_buffer), line_size);
    sl->length = line_size;
    sl->tab_pos = tab_pos;
    return SUCCESS;
}

void free_line(stanza_line_t *stanza_line) {
    free(stanza_line->line);
    free(stanza_line);
}

/* Copies the contents of the next line from file `f`.
 * Increments `l` if a newline is found.
 * Places the size of the line in the location at `line_size`.
 * Places the location of the first tab character at `tab_pos`.
 */
int expect_line(
        unsigned char **line_buffer,
        FILE *f,
        unsigned long *l,
        size_t *line_size,
        int *tab_pos,
        bool is_start) {
    int result = SUCCESS;
    int c;
    size_t spot;
    *tab_pos = -1;
    *line_size = 0UL;
    size_t max_sans_null = MAX_LINE_SIZE - 1;
    for (spot = 0; spot < max_sans_null; spot++) {
        c = fgetc(f);
        if (c == EOF) {
            finish_line(line_buffer, spot, line_size);
            return EOF_REACHED;
        }
        (*line_buffer)[spot] = c;
        switch (c) {
            case '\n':
                (*l)++;
                finish_line(line_buffer, spot, line_size);
                return SUCCESS;
            case '\t':
                if (*tab_pos < 0) {
                    *tab_pos = spot;
                }
                if (*tab_pos == 0 && is_start) {
                    return NO_KEY|START_LINE;
                }
            default:
                title[spot] = c;
        }
    }
    finish_line(line_buffer, spot, line_size);
    return L_TOO_LONG;
}

void free_stanza(stanza_size_t *stanza_buffer) {
    int i;
    for (i = 0; i < MAX_STANZA_SIZE; i++) {
        if (stanza_buffer[i].line != NULL) {
            free_line(stanza_buffer[i].line);
        }
        stanza_buffer[i].line = NULL;
        stanza_buffer[i].length = 0;
        stanza_buffer[i].tab_pos = -1;
    }
}

inline int token_mask(int i) {
    if (i == 0) {
        return START_LINE;
    } else {
        return VAL_CONT;
    }
}

/* stanza_buffer is an array of stanza line pointers of size MAX_STANZA_SIZE.
 * Stanza is a sequence of one or more non-empty lines.
 */
int expect_stanza(FILE *f,
        const unsigned long *l,
        stanza_line_t *stanza_buffer,
        int *last_char) {
    *last_char = 0;
    char line_buffer[MAX_LINE_SIZE];
    int i;
    int tab_pos = -1;
    size_t line_size = 0UL;
    char *line;
    int error;
    int newline_check;
    for (i = 0; i < MAX_STANZA_SIZE; i++) {
        stanza_buffer[i].length = 0;
        stanza_buffer[i].line = NULL;
        stanza_buffer[i].tab_pos = -1;
    }

    for (i = 0; i < MAX_STANZA_SIZE; i++) {
        error = expect_line(&line_buffer, f, l, &line_size, &tab_pos, i == 0);
        if (tab_pos < 0) {
            free_stanza(stanza_buffer);
            return BAD_CHAR|token_mask(i);
        }
        if (error != SUCCESS) {
            free_stanza(stanza_buffer);
            return error|token_mask(i);
        }

        if (line_size == 0UL) {
            if (i == 0) {
                // not strictly necessary, we haven't ran
                // `create_from_line` yet, but since I
                // zeroed out the string pointers
                // above, this should be safe.
                free_stanza(stanza_buffer);
                return INCOMPLETE|token_mask(i);
            }
            // Consume all the empty lines after the stanza
            while ((newline_check = fgetc(f)) != '\n');
            *last_char = newline_check;
            ungetc(newline_check);
            return SUCCESS|token_mask(i);
        }

        error = create_from_line(&stanza_buffer[i], &line_buffer, line_size, tab_pos);
        if (error != SUCCESS) {
            free_stanza(stanza_buffer);
            return error|token_mask(i);
        }
    }
    return TOO_MANY|RECORD;
}


#define MAX_VALUE_SIZE 4096
int find_value(FILE *f, char *primary_key_val, char *select_key, char *value_buffer) {
    int error;
    int i;
    int j = 0;
    int last_char;
    int valpos;
    int left;
    int key_pos = 0;
    unsigned long l = 1;
    unsigned long stanza_start_line = 0;
    bool select_key_found = FALSE;
    bool prikey_val_found = FALSE;
    char *srcval;
    stanza_line_t record[MAX_STANZA_SIZE];
    char key[MAX_LINE_SIZE];
    valpos = 0;
    // skip the schema
    error = expect_stanza(f, &l, TRUE, &schema[0], &last_char);
    if (error != SUCCESS) {
        return error;
    }
    if (last_char == EOF) {
        return EOF_REACHED|SCHEMA;
    }
    while (last_char != EOF) {
        stanza_start_line = line_number;
        error = expect_stanza(f, &l, TRUE, &schema[0], &last_char);
        if (error != SUCCESS) {
            return error;
        }
        for (i = 0; i < MAX_STANZA_SIZE; i++) {
            if (record[i].line == NULL) {
                break;
            }
            if (record[i].tab_pos != 0) {
                if (select_key_found) {
                    // We already recorded the key and value of the last
                    // key, and it's been found, so let's get out of here.
                    return SUCCESS;
                }
                strncpy(&key[0], &(record[i].line[0]), record[i].tab_pos);
                key[tab_pos] = '\0';

                if (i > 0 && key_pos == 0 && strcmp(value, primary_key_val) == 0) {
                    prikey_val_found = TRUE;
                } else {
                    key_pos = i;
                }
                if (strcmp(key, select_key) == 0 && prikey_val_found) {
                    select_key_found = TRUE;
                }
                srcval = tab_pos + 1;
            } else {
                srcval = 1record[i].line[1];
            }

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
            left = min(MAX_VALUE_SIZE - valpos,
                    MAX_LINE_SIZE - record[i].tab_pos);

            for (j = 0; j < left; j++) {
                value_buffer[valpos + j] = record[i].line[srcval + j];
                if (value_buffer[valpos + j] == '\0') {
                    break;
                }
            }

            if (valpos + j >= MAX_VALUE_SIZE) {
                value_buffer[valpos + j - 1] = '\0';
                l = stanza_start_line + i;
                return V_TOO_LONG|VAL_CONT;
            }

            // This error should have already been caught (but I'm
            // suspicious).
            if (srcval + j >= MAX_LINE_SIZE) {
                l = stanza_start_line + i;
                return L_TOO_LONG|VAL_CONT;
            }
            valpos += j;
        }
        if (select_key_found) {
            return SUCCESS;
        }
        if (prikey_val_found) {
            l = stanza_start_line + i;
            return NOT_PRESENT|RECORD;
        }
    }
    return NOT_FOUND|RECORD;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf(stderr, "Usage: %s <filename> <primary-key> <selected-key>\n",
                argv[0]);
        exit(1);
    }
    FILE *ldgrf = fopen(argv[1], "rb");
    unsigned long line_number = 1;
    int result;
    char value_buffer[MAX_VALUE_SIZE];
    result = find_value(ldgrf, &line_number, argv[2], argv[3], &value_buffer);
    if (result != SUCCESS) {
        report_error(result, &line_number);
    }
    close(ldgrf);
    return result;
}