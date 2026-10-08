#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE           0
#define TRUE            1

#define SYMSZ           16

#define CAPPED          20
#define UNCAPPED        30

static int getsym_c = 0;

struct keyval {
        char           *key     ;       // Of length SYMSZ.
        int             val     ;
};

static struct keyval null_pair = {
        .key    =       NULL    ,
        .val    =       0
};

static int read_raw(
        char           *fname   ,
        char          **raw
) {
        FILE *f = fopen(fname, "r");
        if (!f) {
                return FALSE;
        }

        fseek(f, 0, SEEK_END);  // SEEK_END maybe not supported for binary files?
                                // Doesn't matter since won't need this
                                // function if text is passed in.
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

        return ln;
}

static void out_txt(
        char           *txt
) {
        FILE *f = fopen("enc.bin", "wb");
        if (!f) {
                printf("Could not open file to write.\n");
                return;
        }

        // Don't like using strlen really.
        fwrite(txt, sizeof(char), strlen(txt), f);

        fclose(f);
}

static int rawscmp(
        char           *str1    ,
        char           *str2
) {
        char *rstr1 = calloc(strlen(str1), sizeof(char));
        int r1 = 0;
        for (int i = 0; i < strlen(str1); ++i) {
                if (str1[i] >= 'a' && str1[i] <= 'z') {
                        rstr1[r1++] = str1[i];
                } else if (str1[i] >= 'A' && str1[i] <= 'Z') {
                        rstr1[r1++] = str1[i] + 32;
                }
        }

        char *rstr2 = calloc(strlen(str2), sizeof(char));
        int r2 = 0;
        for (int i = 0; i < strlen(str2); ++i) {
                if (str2[i] >= 'a' && str2[i] <= 'z') {
                        rstr2[r2++] = str2[i];
                } else if (str2[i] >= 'A' && str2[i] <= 'Z') {
                        rstr2[r2++] = str2[i] + 32;
                }
        }

        int res = strcmp(rstr1, rstr2);
        free(rstr1);
        free(rstr2);
        rstr1 = rstr2 = NULL;
        return res;
}

static int getsym(
        char           *raw     ,
        char          **sym     ,
        int             bare    // TRUE/FALSE for strip capitals and punc. or not.
) {
        char c;

        char *base = *sym;

        while ((c = raw[getsym_c++])) {  // i.e. while not '\0'
                if (' ' == c) {
                        break;
                } else if (c >= 'a' && c <= 'z') {
                        *(*sym)++ = c;
                } else if (c >= 'A' && c <= 'Z') {
                        *(*sym)++ = c + (32 * bare);
                } else if (!bare && ('-' == c || '.' == c || ',' == c)) {
                        *(*sym)++ = c;
                }
        }
        **sym = '\0';
        *sym = base;
        return strlen(*sym);
}

static void reset_getsym(
        void
) {
        getsym_c = 0;
}

static int count_reps(
        char           *raw     ,
        struct keyval **reps
) {
        int keys = 0;

        char *sym = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while (getsym(raw, &sym, TRUE)) {     // Returns strlen of sym so while >0.
                for (int k = 0; k < keys; ++k) {
                        if (strcmp(sym, (*reps)[k].key)) {
                                continue;
                        }
                        (*reps)[k].val++;
                        goto next_word;
                }

                (*reps)[keys].key = calloc(SYMSZ, sizeof(char));
                strcpy((*reps)[keys].key, sym);
                (*reps)[keys++].val = 1;   // Increment keys for next pair.
next_word:      ;
        }

        free(sym);
        sym = NULL;

        return keys;
}

int main(
        void
) {
        char *raw = NULL;
        if (!read_raw("sample", &raw)) {
                printf("Could not open file.\n");
        }

        // In text editor, raw text would be passed so would start from here.

        // How to decide how much space to allocate for dictionary? Linked list
        // too slow on searching for this, so either has to be a guess, or keep
        // reallocating, or something else... A guess for now. Probably best to
        // do a calculation based on the length of the text, then a realloc
        // just in case.
        int symc = 16;
        struct keyval *repd = calloc(symc, sizeof(struct keyval));
        int keys = count_reps(raw, &repd);

        struct keyval *filt_repd = calloc(keys, sizeof(struct keyval));
        int fkeys = 0;
        int dlen = 0;
        for (int k = 0; k < keys; ++k) {
                if (repd[k].val >= 3) {
                        filt_repd[fkeys++] = repd[k];
                        dlen += strlen(repd[k].key) + 1;
                        repd[k] = null_pair;
                } else {
                        free(repd[k].key);
                        repd[k].key = NULL;
                        repd[k] = null_pair;
                }
        }

        free(repd);
        repd = NULL;

        size_t ln = strlen(raw) + dlen + 3;
        char *out = calloc(ln, sizeof(char));
        char *hd = out;

// Create dictionary in bin.
        for (int k = 0; k < fkeys; ++k) {
                sprintf(hd, "%s\n", filt_repd[k].key);
                hd += strlen(filt_repd[k].key) + 1;
        }
// Delimiter.
        sprintf(hd, "%c%c%c\n", 30, 30, 30);
        hd += 4;
// Encoded text.
        int spc;
        char *w = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while ((getsym(raw, &w, FALSE))) {
                for (int k = 0; k < fkeys; ++k) {
                        if (rawscmp(filt_repd[k].key, w)) {
                                continue;
                        }
                        spc = (w[0] >= 'A' && w[0] <= 'Z') ? CAPPED : UNCAPPED;
                        sprintf(hd, "%c%c ", spc, k + 1);
                        hd += 3;
                        goto cont;
                }
                sprintf(hd, "%s ", w);
                hd += strlen(w) + 1;
cont:           ;
        }

        free(w);
        w = NULL;

        out_txt(out);

        free(out);
        out = hd = NULL;




        // Will deal with capitalisation later.
        // Also leaving out filtering for now, get working for short text
        // first.


        for (int k = 0; k < fkeys; ++k) {
                free(filt_repd[k].key);
                filt_repd[k].key = NULL;
        }

        free(filt_repd);
        filt_repd = NULL;

        free(raw);
        raw = NULL;

        return 0;
}
