#include "util.h"

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
        char          **sym
) {
        static int i = 0;

        char *base = *sym;
        char c;
        do {
                *(*sym)++ = (c = raw[i++]);
        } while ('\n' != c && '\0' != c && ' ' != c);
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
                goto error1;
        }

        char *sym = calloc(128, sizeof(char));
        int slen;

        slen = getsym(raw, &sym);       // Get dictionary length.
        int dlen = atoi(sym);
        if (dlen <= 0) {
                printf("Failed to read dictionary length.\n");
                rt = 1;
                goto error2;
        }

        char **dict = calloc(dlen, sizeof(char *));
        int dci = 0;
        int readdict = TRUE;

        while (0 < (slen = getsym(raw, &sym))) {
                if (readdict) {
                        if (0 == strcmp(sym, DICTSEP "\n")) {
                                readdict = FALSE;
                                continue;
                        }

                        dict[dci] = calloc(SYMSZ, sizeof(char));
                        strcpy(dict[dci], sym);
                        dict[dci++][slen - 1] = '\0';   // Shave off '\n'.
                } else {
                        if (UNCAPPED_C  == sym[0]) {
// Assumes all symbols are 2 bytes (they are, unless I added multi-byte
// extensions for large files).
                                fprintf(out, "%s%s", dict[sym[1] - 1], sym + 2);
                        } else if (CAPPED_C == sym[0]) {
                                fprintf(out, "%c%s%s", dict[sym[1] - 1][0] - 32,
                                                (dict[sym[1] - 1]) + 1,
                                                sym + 2);
                        } else {
                                fprintf(out, "%s", sym);
                        }
                }
        }

        fclose(out);
        out = NULL;

        for (int i = 0; i < dlen; ++i) {
                free(dict[i]);
                dict[i] = NULL;
        }
        free(dict);
        dict = NULL;

error2: free(sym);
        sym = NULL;

error1: free(raw);
        raw = NULL;

        return rt;
}
