#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE   0
#define TRUE    1

#define SYMSZ   16

static int getsym_c = 0;

struct keyval {
        char           *key     ;       // Of length SYMSZ.
        int             val     ;
};

static struct keyval null_pair = {
        .key    =       NULL    ,
        .val    =       0
};

struct ll_sym {
        char           *sym     ;       // Of length SYMSZ.
        struct ll_sym  *next    ;
};

static struct ll_sym * create_sym(
        char           *txt
) {
        // All freed in count_reps loop.
        struct ll_sym *new = malloc(sizeof(struct ll_sym));
        new->sym = calloc(SYMSZ, sizeof(char));
        new->next = NULL;
        strcpy(new->sym, txt);
        return new;
}

static int read_raw(
        char           *fname   ,
        char          **raw
) {
        FILE *f = fopen(fname, "r");
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

static int getsym(
        char           *raw     ,
        char          **sym
) {
        char c;

        char *base = *sym;

        while ((c = raw[getsym_c++])) {  // i.e. while not 0, not null-terminator
                if (' ' == c) {
                        break;
                } else if (c >= 'A' && c <= 'Z') {
                        *(*sym)++ = c + 32;
                } else if ((c >= 'a' && c <= 'z') || '-' == c) {
                        *(*sym)++ = c;
                }       // All other punctuation and numbers ignored.
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

static int scan_syms(
        char           *raw     ,
        struct ll_sym  *sym_root
) {
        struct ll_sym *tail = sym_root;
        char *sym = calloc(SYMSZ, sizeof(char));
        int l = 0;

        reset_getsym();
        while (getsym(raw, &sym)) {     // Returns strlen of sym so while >0.
                if (0 == l++) {         // Increments however condition evals.
                        strcpy(tail->sym, sym);
                        continue;
                }
                tail->next = create_sym(sym);
                tail = tail->next;
        }

        free(sym);
        sym = NULL;

        return l;
}

static int count_reps(
        struct ll_sym  *syms    ,
        struct keyval **reps
) {
        struct ll_sym *next;
        int keys = 0;
        while (NULL != syms) {
                for (int k = 0; k < keys; ++k) {
                        if (strcmp(syms->sym, (*reps)[k].key)) {
                                continue;
                        }
                        (*reps)[k].val++;
                        goto next_word;
                }

                (*reps)[keys].key = calloc(SYMSZ, sizeof(char));
                strcpy((*reps)[keys].key, syms->sym);
                (*reps)[keys++].val = 1;   // Increment keys for next pair.

next_word:      free(syms->sym);
                syms->sym = NULL;
                next = syms->next;
                free(syms);
                syms = next;
        }

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

        struct ll_sym *sym_root = create_sym("");
        int symc = scan_syms(raw, sym_root);

        // Repititions dictionary can have, at most, is same number of keys as
        // there is words, i.e. case of no repititions.
        struct keyval *repd = calloc(symc, sizeof(struct keyval));
        int keys = count_reps(sym_root, &repd);
        // sym_root is now freed and done with

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

        for (int k = 0; k < fkeys; ++k) {
                sprintf(hd, "%s\n", filt_repd[k].key);
                hd += strlen(filt_repd[k].key) + 1;
        }
        sprintf(hd, "%c%c%c\n", 30, 30, 30);
        hd += 4;

        char *w = calloc(SYMSZ, sizeof(char));
        reset_getsym();
        while ((getsym(raw, &w))) {
                for (int k = 0; k < fkeys; ++k) {
                        if (strcmp(filt_repd[k].key, w)) {
                                continue;
                        }
                        sprintf(hd, "%c%c ", 30, k + 1);
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
        

////////////////////////////////////
/*

threes = [key for key in reps if len(key) == 3 and reps[key] >= 5]
fours  = [key for key in reps if len(key) == 4 and reps[key] >= 3]
fives  = [key for key in reps if len(key) == 5 and reps[key] >= 3]
long   = [key for key in reps if len(key) >= 6 and reps[key] >= 2]

if len(threes) + len(fours) + len(fives) + len(long) > 256:
        threes.sort(reverse=True, key=lambda x: reps[x])
        if len(threes) > 256:
                threes = threes[:256]
        fours = [key for key in fours if reps[key] >= 6]
        fives = [key for key in fives if reps[key] >= 4]
        long = [key for key in long if not (len(key) < 8 and 2 == reps[key])]

filtered = threes + fours + fives + long

with open("enc.bin", "wb") as f:
        for j in range(0, len(filtered)):
                i = len(filtered) - 1 - j
                key = f'{chr(i)}'
                # Character with code 92 is '\' which confuses regex.
                if 92 == i:
                        key = '\\' + key

                if i >= 256:
                        esc = 20
                else:
                        esc = 30

                plain = re.sub(f'{filtered[i]}', f'{chr(esc)}{key}', plain)
                plain = re.sub(filtered[i].capitalize(), f'{chr(esc + 1)}{key}', plain)

# Note that this takes the words in reverse order to the above, i.e. uses j not i.
                f.writelines([ord(c).to_bytes(1) for c in f'{filtered[j]}\n'])

        f.write(b'\x1e\x1e\x1e')        # Divider.
        wrt = []
        for c in plain:
                if ord(c) >= 256:
                        wrt.append(ord(c).to_bytes(2))
                else:
                        wrt.append(ord(c).to_bytes(1))

        f.writelines(wrt)
*/
