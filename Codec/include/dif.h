#ifndef DIF_H
#define DIF_H

typedef struct {
    int min;
    int max;
    int nb_bit;
    int offset;
    int prefixe;
    int lgprefixe;
} QauntLvl;

static const QauntLvl DifQuant[4] = {{0, 2, 1, 0, 0x00, 1},
                                               {2, 6, 2, 2, 0x02, 2},
                                               {6, 22, 4, 6, 0x06, 3},
                                               {22, 256, 8, 22, 0x07, 3}};

int pnmtodif(const char *pnminput, const char *difoutput);
int diftopnm(const char *difinput, const char *pnmoutput);

#endif /* DIF_H */