/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "extkeys.h"

/**
 * Reads a line from file f and parses out the key and value from it.
 * The format of a line must match /^ *[A-Za-z0-9_] *[,=] *.+$/.
 * If a line ends in \r, the final \r is stripped.
 * The input file is assumed to have been opened with the 'b' flag.
 * The input file is assumed to contain only ASCII.
 *
 * A line cannot exceed 512 bytes in length.
 * Lines that are excessively long will be silently truncated.
 *
 * On success, *key and *value will be set to point to the key and value in
 * the input line, respectively.
 * *key and *value may also be NULL in case of empty lines.
 * On failure, *key and *value will be set to NULL.
 * End of file is considered failure.
 *
 * Because *key and *value will point to a static buffer, their contents must be
 * copied before calling this function again.
 * For the same reason, this function is not thread-safe.
 *
 * The key will be converted to lowercase.
 * An empty key is considered a parse error, but an empty value is returned as
 * success.
 *
 * This function assumes that the file can be trusted not to contain any NUL in
 * the contents.
 *
 * Whitespace (' ', ASCII 0x20, as well as '\t', ASCII 0x09) at the beginning of
 * the line, at the end of the line as well as around = (or ,) will be ignored.
 *
 * @param f the file to read
 * @param key pointer to change to point to the key
 * @param value pointer to change to point to the value
 * @return 0 on success,
 *         1 on end of file,
 *         -1 on parse error (line too long, line malformed)
 *         -2 on I/O error
 */
static int get_kv(FILE *f, char **key, char **value) {
#define SKIP_SPACE(p)   do {\
    for (; *p == ' ' || *p == '\t'; ++p)\
        ;\
} while(0);
    static char line[1024];
    char *k, *v, *p, *end;

    *key = *value = NULL;

    errno = 0;
    if (fgets(line, (int)sizeof(line), f) == NULL) {
        if (feof(f))
            return 1;
        else
            return -2;
    }
    if (errno != 0)
        return -2;

    if (*line == '\n' || *line == '\r' || *line == '\0')
        return 0;

    /* Not finding \r or \n is not a problem.
     * The line might just be exactly 512 characters long, we have no way to
     * tell.
     * Additionally, it's possible that the last line of a file is not actually
     * a line (i.e., does not end in '\n'); we do want to handle those.
     */
    if ((p = strchr(line, '\r')) != NULL || (p = strchr(line, '\n')) != NULL) {
        end = p;
        *p = '\0';
    } else {
        end = line + strlen(line) + 1;
    }

    p = line;
    SKIP_SPACE(p);
    k = p;

    /* Validate key and convert to lower case. */
    for (; *p != ' ' && *p != ',' && *p != '\t' && *p != '='; ++p) {
        if (*p == '\0')
            return -1;

        if (*p >= 'A' && *p <= 'Z') {
            *p = 'a' + (*p - 'A');
            continue;
        }

        if (*p != '_' &&
                (*p < '0' || *p > '9') &&
                (*p < 'a' || *p > 'z')) {
            return -1;
        }
    }

    /* Bail if the final ++p put us at the end of string */
    if (*p == '\0')
        return -1;

    /* We should be at the end of key now and either whitespace or [,=]
     * follows.
     */
    if (*p == '=' || *p == ',') {
        *p++ = '\0';
    } else {
        *p++ = '\0';
        SKIP_SPACE(p);
        if (*p != '=' && *p != ',')
            return -1;
        *p++ = '\0';
    }

    /* Empty key is an error. */
    if (*k == '\0')
        return -1;

    SKIP_SPACE(p);
    v = p;

    /* Skip trailing whitespace */
    for (p = end - 1; *p == '\t' || *p == ' '; --p)
        ;

    *(p + 1) = '\0';

    *key = k;
    *value = v;

    return 0;
#undef SKIP_SPACE
}

static int ishex(char c) {
    if ('a' <= c && c <= 'f') return 1;
    if ('A' <= c && c <= 'F') return 1;
    if ('0' <= c && c <= '9') return 1;
    return 0;
}

static char hextoi(char c) {
    if ('a' <= c && c <= 'f') return c - 'a' + 0xA;
    if ('A' <= c && c <= 'F') return c - 'A' + 0xA;
    if ('0' <= c && c <= '9') return c - '0';
    return 0;
}

void parse_hex_key(unsigned char *key, const char *hex, unsigned int len) {
    if (strlen(hex) != 2 * len) {
        fatal_error("Key (%s) must be %x hex digits!\n", hex, 2 * len);
    }

    for (unsigned int i = 0; i < 2 * len; i++) {
        if (!ishex(hex[i])) {
            fatal_error("Key (%s) must be %x hex digits!\n", hex, 2 * len);
        }
    }

    memset(key, 0, len);

    for (unsigned int i = 0; i < 2 * len; i++) {
        char val = hextoi(hex[i]);
        if ((i & 1) == 0) {
            val <<= 4;
        }
        key[i >> 1] |= val;
    }
}

void extkeys_initialize_keyset(fusee_extkeys_t *keyset, FILE *f) {
    char *key, *value;
    int ret;
    
    while ((ret = get_kv(f, &key, &value)) != 1 && ret != -2) {
        if (ret == 0) {
            if (key == NULL || value == NULL) {
                continue;
            }
            int matched_key = 0;
            if (strcmp(key, "tsec_root_key") == 0 || strcmp(key, "tsec_root_key_00") == 0) {
                parse_hex_key(keyset->tsec_root_key, value, sizeof(keyset->tsec_root_key));
                matched_key = 1;
            } else {
                char test_name[0x100] = {0};
                for (unsigned int i = 0; i < 0x20 && !matched_key; i++) { 
                    snprintf(test_name, sizeof(test_name), "master_kek_%02x", i);
                    if (strcmp(key, test_name) == 0) {
                        parse_hex_key(keyset->master_keks[i], value, sizeof(keyset->master_keks[i]));
                        matched_key = 1;
                        break;
                    }
                }
            }
        }
    }
}