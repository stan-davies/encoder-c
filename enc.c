#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE           0
#define TRUE            1

#define SYMSZ           16

#define CAPPED          20
#define UNCAPPED        30

struct keyval {
        char           *key     ;       // Of length SYMSZ.
        int             val     ;
};

static struct keyval null_pair = {
        .key    =       NULL    ,
        .val    =       0
};

// Could add repdict, freps, and associated counters to this.
static struct {
        int             dsize           ;       // Symbol dictionary allocated.
        int             getsym_c        ;       // Index for func getsym.
        char           *txt             ;       // Plaintext.
} enc = {
        .dsize          = 0             ,
        .getsym_c       = 0             ,
        .txt            = NULL
};

// Read in plaintext from file.
static int read_raw(
        char           *fname
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

        enc.txt = calloc(ln, sizeof(char));
        int i = 0;
        char c;
        while (EOF != (c = fgetc(f))) {
                enc.txt[i++] = c;
        }
        enc.txt[i] = '\0';

        fclose(f);
        f = NULL;

        return ln;
}

// Write encoded text to binary file.
static void out_txt(
        char           *txt
) {
        FILE *f = fopen("enc.bin", "wb");
        if (!f) {
                printf("Could not open file to write.\n");
                return;
        }
        fwrite(txt, sizeof(char), strlen(txt), f);
        fclose(f);
}

// Variation of strcmp wherein the the strings are first reduced to
// unpunctuated, lowercase text.
static int rawscmp(
        char           *str1    ,
        char           *str2
) {
        char *rstr1 = calloc(strlen(str1), sizeof(char));
        int r1 = 0;
        for (int i = 0; i < (int)strlen(str1); ++i) {
                if (str1[i] >= 'a' && str1[i] <= 'z') {
                        rstr1[r1++] = str1[i];
                } else if (str1[i] >= 'A' && str1[i] <= 'Z') {
                        rstr1[r1++] = str1[i] + 32;
                }
        }

        char *rstr2 = calloc(strlen(str2), sizeof(char));
        int r2 = 0;
        for (int i = 0; i < (int)strlen(str2); ++i) {
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

// Function to be run over in a loop that goes through raw and extracts words
// into sym one at a time. Reset before each run.
static int getsym(
        char          **sym     ,
        int             bare    // TRUE/FALSE for strip capitals and punc. or not.
) {
        char c;
        char *base = *sym;

        // Raise some concern if c exceeds SYMSIZE? Just split in two?
        while ((c = enc.txt[enc.getsym_c++])) {         // i.e. while not '\0'
                if (' ' == c) {
                        break;
                } else if (c >= 'a' && c <= 'z') {
                        *(*sym)++ = c;
                } else if (c >= 'A' && c <= 'Z') {
                        *(*sym)++ = c + (32 * bare);
                } else if (!bare && ('-' == c || '.' == c || ',' == c || '\n' == c)) {
                        *(*sym)++ = c;
                }
        }
        **sym = '\0';
        *sym = base;
        return strlen(*sym);
}

// To reset the above function for next use.
static void reset_getsym(
        void
) {
        enc.getsym_c = 0;
}

// Creates a dictionary of all the words in the given plaintext where the word
// itself is the key and the number of repititions of it is the value.
static int count_reps(
        struct keyval **reps
) {
        int keys = 0;
        void *tmp;  // Used if reps needs reallocating.

        char *sym = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while (getsym(&sym, TRUE)) {    // Returns strlen of sym so while >0.
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
                if (keys == enc.dsize) {
                        enc.dsize += 32;
                        tmp = realloc(*reps, enc.dsize * sizeof(struct keyval));
                        if (NULL == tmp) {
                                printf("Failed to reallocate dicitonary.\n");
                                keys = - (keys - 1);
                                goto end;
                        }
                        *reps = tmp;
                        tmp = NULL;
                }
next_word:      ;
        }
end:    free(sym);
        sym = NULL;

        return keys;
}

// Determine if given symbol is worth encoding.
static int enc_check(
        struct keyval   sym
) {
        int klen = strlen(sym.key);
        return (klen >= 3 && sym.val >= 5)
            || (klen >= 4 && sym.val >= 3)
            || (klen >= 6 && sym.val >= 2);
}

// Filter the dictionary to remove pairs with insufficient repititions.
static int filt_reps(
        struct keyval  *reps    ,
        int             keys    ,
        int            *dlen    ,
        char         ***freps
) {
        int flen = 0;
        for (int k = 0; k < keys; ++k) {
                if (enc_check(reps[k])) {
                        (*freps)[flen] = reps[k].key;  // Passing on ptr.
                        reps[k] = null_pair;
                        *dlen += strlen((*freps)[flen++]) + 1;
                } else {
                        free(reps[k].key);
                        reps[k] = null_pair;
                }
        }
        return flen;
}

// Subsitute marks in place of repeated symbols.
static void subsyms(
        char          **syms    ,
        int             keys    ,
        char          **out
) {
        char *hd = *out;                                        // Print...
        for (int k = 0; k < keys; ++k) {                        // ...dict
                sprintf(hd, "%s\n", syms[k]);
                hd += strlen(syms[k]) + 1;
        }
        sprintf(hd, "%c%c%c\n", 30, 30, 30);                    // ...delimiter
        hd += 4;

        int spc;                                                // ...encoded
        char *w = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while ((getsym(&w, FALSE))) {
                for (int k = 0; k < keys; ++k) {
                        if (rawscmp(syms[k], w)) {
                                continue;
                        }
                        spc = (w[0] >= 'A' && w[0] <= 'Z') ? CAPPED : UNCAPPED;
                        sprintf(hd, "%c%c%s ", spc, k + 1, w + strlen(syms[k]));
                        hd += 3 + strlen(w) - strlen(syms[k]);
                        goto cont;
                }
                sprintf(hd, "%s ", w);
                hd += strlen(w) + 1;
cont:           ;
        }

        *hd = '\0';     // Then strlen is so not a problem.
        hd = NULL;      // This one is getting rid of the pointer reference.

        free(w);
        w = NULL;
}

int encode(
        void
) {
        if (!read_raw("sample")) {
                printf("Could not open file.\n");
                return 1;
        }

        enc.dsize = strlen(enc.txt) / 5;        // Possibly over generous.
        struct keyval *repdict = calloc(enc.dsize, sizeof(struct keyval));
        int dlen = count_reps(&repdict);
        if (dlen < 0) {
                printf("Failed to scan text.\n");
                goto count_err;
        }

        char **freps = calloc(dlen, sizeof(char *));
        int dchars = 0; // Cumulative length of text of each key.
        int flen = filt_reps(repdict, dlen, &dchars, &freps);

        free(repdict);  // Individual pairs freed in the above.
        repdict = NULL;

        char *out = calloc(strlen(enc.txt) + dchars + 4, sizeof(char));
        subsyms(freps, flen, &out);
        out_txt(out);
        free(out);
        out = NULL;

        for (int k = 0; k < flen; ++k) {
                free(freps[k]);
                freps[k] = NULL;
        }
        free(freps);
        freps = NULL;

        free(enc.txt);
        enc.txt = NULL;

        return 0;

count_err:
        for (int i = 0; i < abs(dlen); ++i) {
                free(repdict[i].key);
                repdict[i] = null_pair;
        }
        free(repdict);
        repdict = NULL;

        free(enc.txt);
        enc.txt = NULL;

        return 1;
}
