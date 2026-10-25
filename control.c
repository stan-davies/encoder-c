#include "enc.h"
#include "dec.h"

#include <string.h>
#include <stdio.h>

int main(
        int             argc    ,
        char          **argv
) {
        if (1 == argc) {
                printf("No argument given.\n");
                return 1;
        } else if (0 == strcmp(argv[1], "enc")) {
                printf("encoding...\n");
                return encode();
        } else if (0 == strcmp(argv[1], "dec")) {
                printf("decoding...\n");
                return decode();
        } else {
                printf("No valid argument given.\n");
                return 1;
        }

        return 0;
}
