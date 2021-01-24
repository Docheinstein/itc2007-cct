#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdio.h>
#include <stdbool.h>

#define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define fprint(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)


char *fileread(const char *filename);
int filewrite(const char *filename, bool append, const char *data);
int fileappend(const char *filename, const char *data);
int fileclear(const char *filename);


typedef bool (*parse_line_callback)(
        const char *    /* in: line */,
        void *    /* in: arg */,
        char *    /* out: error_reason */,
        int       /* in: error_reason_length */);

typedef struct fileparse_options {
    char comment_prefix;
} fileparse_options;

char * fileparse(const char *input_file,
                 parse_line_callback callback, void *callback_arg,
                 fileparse_options *options);


#endif // IO_UTILS_H