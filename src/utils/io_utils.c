#include <errno.h>
#include <log/verbose.h>
#include <string.h>
#include <log/debug.h>
#include "io_utils.h"
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
 */
char * fileparse(const char *input_file,
                 parse_line_callback callback,
                 void *callback_arg,
                 fileparse_options *parse_options) {
    static const int MAX_ERROR_LENGTH = 256;
    char error_reason[MAX_ERROR_LENGTH];
    error_reason[0] = '\0';

    static const int MAX_LINE_LENGTH = 256;
    char raw_line[MAX_LINE_LENGTH];
    char *line;
    int line_num = 0;

    verbose2("Opening file: '%s'", input_file);
    FILE *file = fopen(input_file, "r");
    if (!file) {
        snprintf(error_reason, MAX_ERROR_LENGTH,
                 "failed to open '%s' (%s)\n", input_file, strerror(errno));
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

        continue_parsing = callback(line, callback_arg, error_reason, MAX_ERROR_LENGTH);
    }

    verbose2("Closing file: '%s'", input_file);
    if (fclose(file) != 0)
        verbose("WARN: failed to close '%s' (%s)", input_file, strerror(errno));

QUIT:
    if (!strempty(error_reason)) {
        debug("Parser error: %s", error_reason);
        return strmake("parse error at line %d (%s)", line_num, error_reason);
    }

    return NULL; // success
}
