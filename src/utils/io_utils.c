#include <errno.h>
#include "io_utils.h"
#include "mem_utils.h"
#include "str_utils.h"

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

int filewrite(const char *filename, const char *data) {
    if (strempty(data))
        return 0;

    FILE *f = fopen(filename, "wb");
    if (!f)
        return 0;

    int ret = fputs(data, f);
    fclose(f);

    return ret;
}
