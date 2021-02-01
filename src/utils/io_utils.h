#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdio.h>
#include <stdbool.h>

#define print(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define eprint(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

char *fileread(const char *filename);
int filewrite(const char *filename, bool append, const char *data);
int fileappend(const char *filename, const char *data);
int fileclear(const char *filename);

/* Callback passed to `fileparse`.
 * Should return NULL on success, or a malloc-ed string on error. */
typedef char * (*parse_line_callback)(
        const char *    /* in: line */,
        void *          /* in: arg */);

typedef struct fileparse_options {
    char comment_prefix;
} fileparse_options;


/*
 * Returns NULL on success, or a malloc-ed error string on failure.
 * `callback` is responsible for the parsing of the lines.
 */
char * fileparse(const char *filename, fileparse_options *options,
                 parse_line_callback callback, void *callback_arg);


#endif // IO_UTILS_H