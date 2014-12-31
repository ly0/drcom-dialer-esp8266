#include "esp_common.h"
#include <ctype.h>
/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no moretokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char ICACHE_FLASH_ATTR
*strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}


#define __LONG_LONG_MAX__ 9223372036854775807LL
#define LLONG_MAX __LONG_LONG_MAX__
#define ULLONG_MAX (LLONG_MAX * 2ULL + 1)

unsigned long long ICACHE_FLASH_ATTR
strtoull(const char *nptr, char **endptr, int base)
{
    const char *s;
    unsigned long long acc, cutoff;
    int c;
    int neg, any, cutlim;

    /*
     * See strtoll for comments as to the logic used.
     */
    if (base < 0 || base == 1 || base > 36) {
        if (endptr != 0)
            *endptr = (char *)nptr;
        errno = EINVAL;
        return 0;
    }

    s = nptr;
    do {
        c = (unsigned char) *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    cutoff = ULLONG_MAX / (unsigned long long)base;
    cutlim = ULLONG_MAX % (unsigned long long)base;
    for (acc = 0, any = 0;; c = (unsigned char) *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0)
            continue;
        if (acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
            acc = ULLONG_MAX;
            errno = ERANGE;
        } else {
            any = 1;
            acc *= (unsigned long long)base;
            acc += c;
        }
    }
    if (neg && any > 0)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}
