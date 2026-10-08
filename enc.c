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

// Read in plaintext from file.
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

// To reset the above function for next use.
static void reset_getsym(
        void
) {
        getsym_c = 0;
}

// Creates a dictionary of all the words in the given plaintext where the word
// itself is the key and the number of repititions of it is the value.
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

// Filter the dictionary to remove pairs with insufficient repititions.
static int filt_reps(
        struct keyval  *reps    ,
        int             keys    ,
        int            *dlen    ,
        struct keyval **freps
) {
        int fkeys = 0;
        for (int k = 0; k < keys; ++k) {
                if (reps[k].val >= 3) {
                        (*freps)[fkeys++] = reps[k];
                        *dlen += strlen(reps[k].key) + 1;
                        reps[k] = null_pair;
                } else {
                        free(reps[k].key);
                        reps[k].key = NULL;
                        reps[k] = null_pair;
                }
        }
        return fkeys;
}

// Subsitute marks in place of repeated symbols.
static void subsyms(
        char           *raw     ,
        struct keyval  *dict    ,
        int             keys    ,
        char          **out
) {
        char *hd = *out;                                        // Print...
        for (int k = 0; k < keys; ++k) {                        // ...dict
                sprintf(hd, "%s\n", dict[k].key);
                hd += strlen(dict[k].key) + 1;
        }
        sprintf(hd, "%c%c%c\n", 30, 30, 30);                    // ...delimiter
        hd += 4;

        int spc;                                                // ...encoded
        char *w = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while ((getsym(raw, &w, FALSE))) {
                for (int k = 0; k < keys; ++k) {
                        if (rawscmp(dict[k].key, w)) {
                                continue;
                        }
                        spc = (w[0] >= 'A' && w[0] <= 'Z') ? CAPPED : UNCAPPED;
                        sprintf(hd, "%c%c%s ", spc, k + 1, w + strlen(dict[k].key));
                        hd += 3 + strlen(w) - strlen(dict[k].key);
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

int main(
        void
) {
        char *raw = NULL;
        if (!read_raw("sample", &raw)) {
                printf("Could not open file.\n");
        }

        // In text editor, raw text would be passed so would start from here.

        int keys;

        // How to decide how much space to allocate for dictionary? Linked list
        // too slow on searching for this, so either has to be a guess, or keep
        // reallocating, or something else... A guess for now. Probably best to
        // do a calculation based on the length of the text, then a realloc
        // just in case.
        int symc = 16;
        struct keyval *repdict = calloc(symc, sizeof(struct keyval));
        keys = count_reps(raw, &repdict);

        struct keyval *frepdict = calloc(keys, sizeof(struct keyval));
        int dlen = 0;   // Cumulative length of text of each key.
        keys = filt_reps(repdict, keys, &dlen, &frepdict);

        free(repdict);  // Individual pairs freed in the above.
        repdict = NULL;

        char *out = calloc(strlen(raw) + dlen + 4, sizeof(char));
        subsyms(raw, frepdict, keys, &out);
        out_txt(out);
        free(out);
        out = NULL;

        for (int k = 0; k < keys; ++k) {
                free(frepdict[k].key);
                frepdict[k].key = NULL;
        }
        free(frepdict);
        frepdict = NULL;

        free(raw);
        raw = NULL;

        return 0;
}
