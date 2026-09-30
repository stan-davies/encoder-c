#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE   0
#define TRUE    1

#define SYMSZ   16

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

static int getsym(
        char           *raw     ,
        char          **sym
) {
        static int i = 0;       // Only used once per run.
        char c;

        char *base = *sym;

        while ((c = raw[i++])) {  // i.e. while not 0, not null-terminator
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
                        
static void scan_syms(
        char           *raw     ,
        char         ***syms
) {
        char **base = *syms;
        char *sym = calloc(SYMSZ, sizeof(char));

        while (getsym(raw, &sym)) {     // Returns strlen of sym so while >0.
                strcpy(*(*syms)++, sym);
        }

        free(sym);
        sym = NULL;

        *syms = base;
}

int main(
        void
) {
        char *raw;

        if (!read_raw("sample", &raw)) {
                printf("Could not open file.\n");
        }

                // Space for 16 strings.
        char **syms = calloc(16, sizeof(char *));
        for (int i = 0; i < 16; ++i) {
                // Each string 16 characters max.
                syms[i] = calloc(SYMSZ, sizeof(char));
        };

        scan_syms(raw, &syms);

        for (int i = 0; i < 16; ++i) {
                printf("%d:\t'%s'\n", i, syms[i]);
        }


        // Will deal with capitalisation later.


        return 0;
}
        

////////////////////////////////////
/*

reps = {}
for s in symbols:
        if s.lower() in reps:
                reps[s.lower()] += 1
        else:
                reps[s.lower()] = 1

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
