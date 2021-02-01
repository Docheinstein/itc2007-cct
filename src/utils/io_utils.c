#include "io_utils.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "log/verbose.h"
#include "log/debug.h"
#include "mem_utils.h"
#include "str_utils.h"
#include "array_utils.h"

char *fileread(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = mallocx(filesize + 1, sizeof(char));
    fread(content, sizeof(char), filesize, f);
    fclose(f);

    content[filesize] = '\0';
    return content; // must be freed outside
}

int filewrite(const char *filename, bool append, const char *data) {
    if (!data)
        return 0;

    FILE *f = append ? fopen(filename, "ab") : fopen(filename, "wb");
    if (!f)
        return 0;

    int ret = fputs(data, f);
    fclose(f);

    return ret;
}

int fileappend(const char *filename, const char *data) {
    return filewrite(filename, true, data);
}

int fileclear(const char *filename) {
    return filewrite(filename, false, "");
}

/*
 * Returns NULL on success, or a malloc-ed error string on failure.
 * `callback` is responsible for the parsing of the lines.
 */
char * fileparse(const char *filename,
                 fileparse_options *parse_options,
                 parse_line_callback callback,
                 void *callback_arg) {
    static const int MAX_ERROR_LENGTH = 256;
    char error_reason[MAX_ERROR_LENGTH];
    error_reason[0] = '\0';

    static const int MAX_LINE_LENGTH = 256;
    char raw_line[MAX_LINE_LENGTH];
    char *line;
    int line_num = 0;

    verbose("Opening file: '%s'", filename);
    FILE *file = fopen(filename, "r");
    if (!file) {
        snprintf(error_reason, MAX_ERROR_LENGTH,
                 "failed to open '%s' (%s)\n", filename, strerror(errno));
        goto QUIT;
    }

    bool continue_parsing = true;
    char comment_prefix = '\0';
    if (parse_options)
        comment_prefix = parse_options->comment_prefix;

    while (continue_parsing && fgets(raw_line, LENGTH(raw_line), file)) {
        line_num++;
        line = strtrim(raw_line);
        verbose2("<< %s", line);

        if (strempty(line) || strstarts(line, comment_prefix))
            continue;

        char *err = callback(line, callback_arg);
        if (err) {
            continue_parsing = false;
            snprintf(error_reason, MAX_ERROR_LENGTH, "%s", err);
            free(err);
        }
    }

    verbose("Closing file: '%s'", filename);
    if (fclose(file) != 0)
        verbose("WARN: failed to close '%s' (%s)", filename, strerror(errno));

QUIT:
    if (!strempty(error_reason)) {
        verbose("-- PARSE FAILURE --\n"
                "Error reason: %s\n"
                "Line number: %d\n"
                "Line: %s", error_reason, line_num, line);
        return strmake("parse error at line %d (%s)", line_num, error_reason);
    }

    return NULL; // success
}
