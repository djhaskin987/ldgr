#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS     0x0
#define BAD_CHAR     0x1
#define IO_ERROR     0x2
#define INCOMPLETE   0x4
#define EOF_REACHED  0x8
#define TOO_LONG    0x10
#define TOO_MANY    0x20
#define NO_MEM      0x40
#define UNKNOWN     0x80
#define ERROR_MASK  0xFF

#define TITLE        0x100
#define DESCRIPTION  0x200
#define PREAMBLE     0x400
#define START_LINE   0x800
#define VAL_CONT    0x1000
#define RECORD      0x2000
#define FORM        0x4000
#define TOKEN_MASK  0x7F00

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
        case TOO_LONG:
            return "line was too long (must be 511 characters or less)";
        case NO_MEM:
            return "memory allocation error";
        case UNKNOWN:
            return "unknown error";
    }
    return NULL;
}

const char *error_token(int error) {
    switch (error & TOKEN_MASK) {
        case TITLE:
            return "title";
            break;
        case DESCRIPTION:
            return "description";
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
        case FORM:
            return "form";
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


inline int finish_line(char **line_buffer, size_t spot, size_t *line_size) {
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

free_line(stanza_line_t *stanza_line) {
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
        int *tab_pos) {
    int result = SUCCESS;
    int c;
    size_t spot;
    *tab_pos = -1;
    *line_size = 0UL;
    size_t max_sans_null = MAX_LINE_SIZE - 1;
    for (spot = 0; spot < max_sans_null; spot++) {
        c = fgetc(f);
        if (c == EOF) {
            finish_line(line_buffer, spot, line_size, put);
            return EOF_REACHED;
        }
        (*line_buffer)[spot] = c;
        switch (c) {
            case '\n':
                (*l)++;
                finish_line(line_buffer, spot, line_size, put);
                return SUCCESS;
            case '\t':
                if (*tab_pos < 0) {
                    *tab_pos = spot;
                }
            default:
                title[spot] = c;
        }
    }
    finish_line(line_buffer, spot, line_size, put);
    return TOO_LONG;
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

int token_mask(int i, int is_preamble) {
    if (is_preamble) {
        if (i == 0) {
            return TITLE;
        } else {
            return DESCRIPTION;
        }
    } else {
        if (i == 0) {
            return START_LINE;
        } else {
            return VAL_CONT;
        }
    }
}

/* stanza_buffer is an array of stanza line pointers of size MAX_STANZA_SIZE.
 * Stanza is a sequence of one or more non-empty lines.
 */
int expect_stanza(stanza_line_t *stanza_buffer, FILE *f, const unsigned long *l, int is_preamble) {
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
        error = expect_line(&line_buffer, f, l, &line_size, &tab_pos);
        if ((tab_pos >= 0 && is_preamble) ||
            (tab_pos < 0 && !is_preamble)) {
            free_stanza(stanza_buffer);
            return BAD_CHAR|token_mask(i, is_preamble);
        }
        if (error != SUCCESS) {
            free_stanza(stanza_buffer);
            return error|token_mask(i, is_preamble);
        }

        if (line_size == 0UL) {
            if (i == 0) {
                // not strictly necessary, we haven't ran
                // `create_from_line` yet, but since I
                // zeroed out the string pointers
                // above, this should be safe.
                free_stanza(stanza_buffer);
                return INCOMPLETE|token_mask(i, is_preamble);
            }
            // Consume all the empty lines after the stanza
            while ((newline_check = fgetc(f)) != '\n');
            ungetc(newline_check);
            return SUCCESS|token_mask(i, is_preamble);
        }

        error = create_from_line(&stanza_buffer[i], &line_buffer, line_size, tab_pos);
        if (error != SUCCESS) {
            free_stanza(stanza_buffer);
            return error|token_mask(i, is_preamble);
        }
    }
    return TOO_MANY|(is_preamble ? PREAMBLE : RECORD);
}


typedef struct {
    char *key, 
} ;

int create_table(FILE *f, unsigned long *l) {
    int error;
    int i;
    stanza_line_t preamble[MAX_STANZA_SIZE];
    stanza_line_t schema[MAX_STANZA_SIZE];
    error = expect_stanza(&preamble[0], f, l, TRUE);
    if (error != SUCCESS) {
        return error;
    }
    expect_stanza(&schema[0], f, l, TRUE);
    if (error != SUCCESS) {
        return error;
    }
    // Print the description as comments above the statement.
    for (i = 1; i < MAX_STANZA_SIZE; i++) {
        if (preamble[i]->line == NULL) {
            break;
        }
        printf("-- %s\n", preamble[1]->line);
    }
    // I can expect at least one line in preamble,
    // otherwise an INCOMPLETE error would have been
    // returned.
    printf("CREATE TABLE %s (\n\t", preamble[0]->line);
    for (i = 0; i < MAX_STANZA_SIZE; i++) {
        if (schema[i]->line == NULL) {
            break;
        }
        printf("\t%s %s,\n", scheam-- %s\n", preamble[1]->line);
    }




}

int convert_title(FILE *f, const unsigned long *l) {
    int result = 0;
    unsigned char title[MAX_LINE_SIZE];

    int spot;
    unsigned char c;
    if (c == EOF) {
        return EOF_REACHED|TITLE;
    }
    for (spot = 0; spot < MAX_LINE_SIZE; spot++) {
        c = fgetc(f);
        title[spot] = c;
        if (c == '\r') {
            c = fgetc(f);
            if (c == EOF) {
                title[spot] = '\0';
                // Form complete, but has zero records
                break;
            } else if (c != '\n') {
                title[spot] = '\0';
                ungetc(c, f);
                break;
            } else {
                title[spot] = '\0';
                break;
            }
        } else if (c == '\n') {
            title[spot] = '\0';

            break;
        } else if (c == '\t') {
            return BAD_CHAR|TITLE;
        }
    }
    if (spot == MAX_LINE_SIZE) {
k:wq







        if (c == '\n') {



        c
        kllkk
    while (c != EOF &&




int convert_preamble(FILE *f, const unsigned long *l) {
    int result = 0;
    result = convert_title(f, l);
    while (result == 0) {
        result = convert_description_line(f, l);
    }
    return result;
}

int convert_form(FILE *f, const unsigned long *l) {
    int result = 0;
    while (result == 0) {
        result = convert_preamble(f, l);
        if (result == 0) {
            result = convert_records(f, l);
        }
    }
    return result;
}

int convert(char *file) {
    int result = 0;
    FILE *f = fopen(file, "rb");
    unsigned long line_number = 0UL;
    while (result == 0) {
        result = convert_form(f,&l);
    }
    fclose(f);
    switch (result) {
        case NO_CONTENT:
            fprintf(stderr, "you\n
            fprintf(stderr, "No content in file.");
            break;
        case IO_ERROR:
            fprintf(stderr, "Couldn't read file."); break;
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(stderr, "Please provide the name of a file as an argument.");
        exit(1);
    }
    FILE *wctnf = fopen(argv[1], "rb");

    form_t *forms = NULL;
    int result = convert(argv[1]);
    return result;
}