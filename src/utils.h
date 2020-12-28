#ifndef UTILS_H
#define UTILS_H

#define LENGTH(a) (sizeof(a) / sizeof(a[0]))

#define streq(s1, s2) (strcmp(s1, s2) == 0)
int strpos(const char *str, char character);


char * strrtrim(char *str);
char * strltrim(char *str);
char * strtrim(char *str);

#endif // UTILS_H