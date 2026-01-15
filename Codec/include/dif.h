#ifndef DIF_H
#define DIF_H
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int min;
    int max;
    int nb_bit;
    int offset;
    int prefixe;
    int lgprefixe;
} QauntLvl;

typedef struct {
    const unsigned char *data;
    size_t size;
    size_t index;
    int bit_pos;
} BitReader;

typedef struct {
    unsigned char *data;
    size_t index;
    size_t size;
    unsigned char wait;
    int n_wait;
} BitBuffer;


const QauntLvl DifQuant[4] = {{0, 2, 1, 0, 0x00, 1},
                                               {2, 6, 2, 2, 0x02, 2},
                                               {6, 22, 4, 6, 0x06, 3},
                                               {22, 256, 8, 22, 0x07, 3}};

int pnmtodif(char *pnminput, char *difoutput);
int diftopnm(char *difinput, char *pnmoutput);

#endif /* DIF_H */