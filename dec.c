#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FALSE           0
#define TRUE            1

// Read in file as binary.
// Split on the divider.
// Split dictionary on newlines, creating array of substitutions.

static int read_enc(
        char           *fname   ,
        char          **raw
) {
        FILE *f = fopen(fname, "rb");
        if (!f) {
                return FALSE;
        }

        fseek(f, 0, SEEK_END);  // SEEK_END maybe not supported for binary files?
        int ln = ftell(f) + 1;  // +1 for '\0'
        rewind(f);

        *raw = calloc(ln, sizeof(char));
        int i = 0;
        char c;
        while (EOF != (c = fgetc(f))) {
                (*raw)[i++] = c;
        }
        (*raw)[i] = '\0';

        fclose(f);
        f = NULL;

        return TRUE;
}

static int getsym(
        char           *raw     ,
        char          **sym     ,
        char            delim
) {
        static int i = 0;

        char *base = *sym;
        char c;
        while (delim != (c = raw[i++]) && '\0' != c) {
                *(*sym)++ = c;
        }
        **sym = '\0';
        *sym = base;
        return strlen(*sym);
}

int decode(
        void
) {
        int rt = 0;

        char *raw;
        if (!read_enc("enc.bin", &raw)) {
                printf("Could not read encoded text.\n");
                return 1;
        }

        FILE *out = fopen("decoded", "w");
        if (!out) {
                printf("Could not open output file.\n");
                rt = 1;
                goto error;
        }

        // Insert dictionary length in file?
        char **dict = calloc(16, sizeof(char *));
        int dci = 0;

        char *sym = calloc(128, sizeof(char));
        int slen;
        char delim = '\n';
        while (0 < (slen = getsym(raw, &sym, delim))) {
                if ('\n' == delim) {
                        dict[dci] = calloc(16, sizeof(char));
                        strcpy(dict[dci++], sym);
                } else {
                        if ('\x1e'  == sym[0]) {
                                fprintf(out, "%s ", dict[sym[1] - 1]);
                        } else if ('\x14' == sym[0]) {
                                fprintf(out, "%c%s ", dict[sym[1] - 1][0] - 32,
                                                (dict[sym[1] - 1]) + 1);
                        } else {
                                fprintf(out, "%s ", sym);
                        }
                }

                if (0 == strcmp(sym, "\x1e\x1e\x1e")) {
                        delim = ' ';
                }
        }
        fprintf(out, "\n");

        free(sym);
        sym = NULL;

        fclose(out);
        out = NULL;

error:  free(raw);
        raw = NULL;

        return rt;
}

/*

plain = ''
esc = False
cap = False
dub = False

for i in range(0, len(enc)):
        c = enc[i]
        if esc:
                if not dub:
                        word = subs[ord(c)]
                else:
                        word = subs[(ord(c) << 8) + ord(enc[i + 1])]

                if cap:
                        plain += word.capitalize()
                else:
                        plain += word
                esc = False
        elif dub:       # Prevent second character in index being reread.
                dub = False
                continue
        elif '\x1e' == c:
                esc = True
                cap = False
                dub = False
        elif '\x1f' == c:
                esc = True
                cap = True
                dub = False
        elif '\x14' == c:
                esc = True
                cap = False
                dub = True
        elif '\x15' == c:
                esc = True
                cap = True
                dub = True
        else:
                plain += c

with open('dec', 'w') as f:
        f.write(plain)
*/
