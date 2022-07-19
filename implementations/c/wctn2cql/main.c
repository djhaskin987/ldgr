#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned char *key;
    unsigned char *val;
} field_t;

typedef struct {
    field_t *fields;
} record_t;

typedef struct {
    unsigned char *title;
    unsigned char *description;
} preamble_t;

typedef struct {
    preamble_t preamble;
    record_t *records;
} form_t;

typedef int parse_t;
#define SUCCESS     0x0
#define BAD_CHAR    0x1
#define IO_ERROR    0x2
#define INCOMPLETE  0x4
#define EOF_REACHED 0x8
#define TOO_BIG     0x10
#define ERROR_MASK  0x1F

#define TITLE       0x20
#define DESCRIPTION 0x40
#define KEY         0x80
#define VALUE       0x100
#define FORM        0x200
#define RECORD      0x400
#define TOKEN_MASK  0x7E0

#define MAX_LENGTH 65

const char *error_error(int error) {
    switch (error & ERROR_MASK) {
        case SUCCESS:
            return "successful";
            break;
        case BAD_CHAR:
            return "bad character";
            break;
        case IO_ERROR:
            return "I/O error";
            break;
        case INCOMPLETE:
            return "incomplete";
            break;
        case EOF_REACHED:
            return "EOF reached";
        case TOO_BIG:
            return "Line was too long (must be 65 characters or less)";
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
        case KEY:
            return "key";
            break;
        case VALUE:
            return "value";
            break;
        case FORM:
            return "form";
            break;
        case RECORD:
            return "record";
            break;

    }
    return NULL;
}

// The maximum number of characters, INCLUDING NULL TERMINAL AND LINE
// DELIMITER, a line can be.
#define MAX_LINE_SIZE 512

// Takes a simple lval and a maybe-complex rval


/* Takes a file, a pointer to an integer for recording the tab
 * position, and a pointer to an unsigned for recording the line number.
 * Records the position of the first tab character, if present.
 *
 */
#define EXPLINE_SUCCESS 0
#define EXPLINE_NOT_FOUND -1
#define EXPLINE_EOF_REACHED -2
#define EXPLINE_TOO_LONG -3
/* Fills the `line` buffer below with the contents of the next line.
 * Accepts Windows, Linux, or old Mac line endings.
 */
unsigned char line[MAX_LINE_SIZE];
int expect_line(FILE *f, int *tab_pos, size_t *line_size, unsigned long *l) {
    int result = SUCCESS;
    int c;
    size_t spot;
    *tab_pos = EXPLINE_NOT_FOUND;
    *line_size = 0UL;
    size_t max_sans_null = MAX_LINE_SIZE - 1;
    for (spot = 0; spot < max_sans_null; spot++) {
        c = fgetc(f);
        if (c == EOF) {
            line[spot] = '\0';
            *line_size = spot;
            return EXPLINE_EOF_REACHED;
        }
        line[spot] = c;
        switch (c) {
            case '\n':
                (*l)++;
                spot++;
                line[spot] = '\0';
                *line_size = spot;
                return EXPLINE_SUCCESS;
            case '\t':
                if (*tab_pos < 0) {
                    *tab_pos = spot;
                }
            default:
                title[spot] = c;
        }
    }
    line[spot] = '\0';
    return EXPLINE_TOO_LONG;
}

typedef struct {
    char *table_name;
} create_table_t;

int convert_title 

int convert_title(FILE *f, const unsigned long *l) {
    int result = 0;
    unsigned char title[MAX_LINE_SIZE];

    int spot;
    unsigned char c;
    if (c == EOF) {
        return INCOMPLETE|TITLE;
    }iplayedwifNaph-an4
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