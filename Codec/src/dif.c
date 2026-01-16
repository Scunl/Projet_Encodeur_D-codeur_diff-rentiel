#include "../include/dif.h"

int fold_diff(int diff) {
    if (diff < 0)
        return ((-2) * diff) - 1;
    return 2 * diff;
}

int unfold_diff(int val) {
    if (val % 2 == 1)
        return -(val + 1) / 2;
    return val / 2;
}

void skip_comments(FILE *f) {
    int ch;
    char buffer[1024];

    while ((ch = fgetc(f)) != EOF) {
        if (isspace(ch)) {
            continue;
        }
        if (ch == '#') {
            if (fgets(buffer, sizeof(buffer), f) == NULL)
                return;
            continue;
        }
        ungetc(ch, f);

        return;
    }
}

void init_buffer(BitBuffer *bf, unsigned char *ptr, size_t size) {
    bf->data = ptr;
    bf->index = 0;
    bf->size = size;
    bf->wait = 0;
    bf->n_wait = 0;
}

void push_bits(BitBuffer *bf, int val, int nb) {
    for (int i = nb - 1; i >= 0; i--) {
        int bit = (val >> i) & 1;

        bf->wait = (bf->wait << 1) | bit;
        bf->n_wait++;

        if (bf->n_wait == 8) {
            if (bf->index < bf->size) {
                bf->data[bf->index] = bf->wait;
                bf->index++;
            }
            bf->wait = 0;
            bf->n_wait = 0;
        }
    }
}

void flush_buffer(BitBuffer *bf) {
    if (bf->n_wait > 0) {
        bf->wait = bf->wait << (8 - bf->n_wait);

        if (bf->index < bf->size) {
            bf->data[bf->index] = bf->wait;
            bf->index++;
        }
        bf->n_wait = 0;
    }
}

int pnmtodif(char *pnminput, char *difoutput) {
    FILE *f_in = NULL;
    FILE *f_out = NULL;
    char magic[3];
    int width, height, maxval;
    int is_rgb = 0;

    f_in = fopen(pnminput, "rb");
    if (!f_in) {
        perror("Erreur ouverture input");
        return 1;
    }

    if (fscanf(f_in, "%2s", magic) != 1) {
        fclose(f_in);
        return 2;
    }

    if (strcmp(magic, "P6") == 0) {
        is_rgb = 1;
    } else if (strcmp(magic, "P5") == 0) {
        is_rgb = 0;
    } else {
        fprintf(stderr, "Format erreur\n");
        fclose(f_in);
        return 3;
    }
    skip_comments(f_in);
    if (fscanf(f_in, "%d %d %d", &width, &height, &maxval) != 3) {
        fclose(f_in);
        return 2;
    }
    fgetc(f_in);

    f_out = fopen(difoutput, "wb");
    if (!f_out) {
        perror("Erreur ouverture output");
        fclose(f_in);
        return 1;
    }

    unsigned short dif_magic = is_rgb ? 0xD3FF : 0xD1FF;
    fwrite(&dif_magic, sizeof(unsigned short), 1, f_out);

    unsigned short w = (unsigned short)width;
    unsigned short h = (unsigned short)height;
    fwrite(&w, sizeof(unsigned short), 1, f_out);
    fwrite(&h, sizeof(unsigned short), 1, f_out);

    unsigned char n_levels = 4;
    fwrite(&n_levels, sizeof(unsigned char), 1, f_out);

    unsigned char bits_per_level[4] = {1, 2, 4, 8};
    fwrite(bits_per_level, sizeof(unsigned char), 4, f_out);

    int num_layers = is_rgb ? 3 : 1;
    size_t num_pixels = (size_t)width * height * num_layers;
    size_t buffer_size = (size_t)(1.375 * num_pixels) + 1024;

    unsigned char *raw_data = malloc(buffer_size);
    if (!raw_data) {
        fprintf(stderr, "Erreur allocation mémoire\n");
        fclose(f_in);
        fclose(f_out);
        return 4;
    }
    BitBuffer bf;
    init_buffer(&bf, raw_data, buffer_size);

    unsigned char *img_data = malloc(num_pixels);
    if (!img_data) {
        perror("Erreur allocation image");
        free(raw_data);
        fclose(f_in);
        fclose(f_out);
        return 4;
    }
    
    if (fread(img_data, 1, num_pixels, f_in) != num_pixels) {
        fprintf(stderr, "Erreur lectureu\n");
        free(img_data);
        free(raw_data);
        fclose(f_in);
        fclose(f_out);
        return 5;
    }

    int channels = is_rgb ? 3 : 1;
    int prev_reduced[3];

    for (int k = 0; k < channels; k++) {
        unsigned char val = img_data[k];
        fwrite(&val, 1, 1, f_out);
        prev_reduced[k] = val / 2;
    }

    for (size_t i = channels; i < num_pixels; i++) {
        int chan = i % channels;
        unsigned char val_brute = img_data[i];
        int val_reduite = val_brute / 2;

        int diff = val_reduite - prev_reduced[chan];

        prev_reduced[chan] = val_reduite;

        int folded = fold_diff(diff);
        for (int q = 0; q < 4; q++) {
            if (folded >= DifQuant[q].min && folded < DifQuant[q].max) {
                push_bits(&bf, DifQuant[q].prefixe, DifQuant[q].lgprefixe);
                int val_to_code = folded - DifQuant[q].offset;
                push_bits(&bf, val_to_code, DifQuant[q].nb_bit);
                break;
            }
        }
    }

    free(img_data);

    printf("Header écrit et l'encodage de %dx%d pixels.\n", width, height);
    flush_buffer(&bf);
    fwrite(bf.data, 1, bf.index, f_out);
    free(raw_data);
    fclose(f_in);
    fclose(f_out);

    return 0;
}

/*A changer en bas*/

void init_reader(BitReader *br, unsigned char *data, size_t size) {
    br->data = data;
    br->size = size;
    br->index = 0;
    br->bit_pos = 7;
}

int pull_bit(BitReader *br) {
    if (br->index >= br->size)
        return 0;

    int bit = (br->data[br->index] >> br->bit_pos) & 1;

    br->bit_pos--;
    if (br->bit_pos < 0) {
        br->bit_pos = 7;
        br->index++;
    }
    return bit;
}

int pull_bits(BitReader *br, int nb) {
    int val = 0;
    for (int i = 0; i < nb; i++) {
        val = (val << 1) | pull_bit(br);
    }
    return val;
}

int diftopnm(char *difinput, char *pnmoutput) {
    FILE *f_in = fopen(difinput, "rb");
    if (!f_in) {
        perror("Erreur ouverture input DIF");
        return 1;
    }

    unsigned short magic, w, h;
    if (fread(&magic, 2, 1, f_in) != 1) {
        fclose(f_in);
        return 2;
    }

    int is_rgb = 0;
    if (magic == 0xD3FF)
        is_rgb = 1;
    else if (magic == 0xD1FF)
        is_rgb = 0;
    else {
        fprintf(stderr, "Erreur: Ce n'est pas un fichier DIF valide.\n");
        fclose(f_in);
        return 3;
    }

    if (fread(&w, 2, 1, f_in) != 1) {
        fclose(f_in);
        return 2;
    }
    if (fread(&h, 2, 1, f_in) != 1) {
        fclose(f_in);
        return 2;
    }

    printf("h %d w %d\n", h, w);

    unsigned char q_dummy[5];
    if (fread(q_dummy, 1, 5, f_in) != 5) {
        fprintf(stderr, "Erreur: Fichier DIF tronqué (header).\n");
        fclose(f_in);
        return 2;
    }

    FILE *f_out = fopen(pnmoutput, "wb");
    if (!f_out) {
        perror("Erreur ouverture output PNM");
        fclose(f_in);
        return 1;
    }

    fprintf(f_out, "%s\n%d %d\n255\n", is_rgb ? "P6" : "P5", w, h);

    long header_pos = ftell(f_in);
    fseek(f_in, 0, SEEK_END);
    long file_size = ftell(f_in);
    fseek(f_in, header_pos, SEEK_SET);

    size_t data_size = file_size - header_pos;
    unsigned char *compressed_data = malloc(data_size);
    if (!compressed_data) {
        fclose(f_in);
        fclose(f_out);
        return 4;
    }

    if (fread(compressed_data, 1, data_size, f_in) != data_size) {
        fprintf(stderr,
                "Erreur: Lecture incomplète des données compressées.\n");
        free(compressed_data);
        fclose(f_in);
        fclose(f_out);
        return 4;
    }
    BitReader br;
    init_reader(&br, compressed_data, data_size);

    int channels = is_rgb ? 3 : 1;
    size_t num_pixels = (size_t)w * h * channels;

    unsigned char *img_out = malloc(num_pixels);
    if (!img_out) {
        free(compressed_data);
        fclose(f_in);
        fclose(f_out);
        return 4;
    }

    int prev_reduced[3];

    for (int k = 0; k < channels; k++) {

        unsigned char val = compressed_data[br.index];
        br.index++;

        img_out[k] = val;
        prev_reduced[k] = val / 2;
    }

    for (size_t i = channels; i < num_pixels; i++) {
        int chan = i % channels;

        int q_index = 0;

        if (pull_bit(&br) == 0) {
            q_index = 0;
        } else {
            if (pull_bit(&br) == 0) {
                q_index = 1;
            } else {
                if (pull_bit(&br) == 0) {
                    q_index = 2;
                } else {
                    q_index = 3;
                }
            }
        }

        int nb_bits = DifQuant[q_index].nb_bit;
        int val_code = pull_bits(&br, nb_bits);

        int folded = val_code + DifQuant[q_index].offset;

        int diff = unfold_diff((signed char)folded);

        int val_reduite = prev_reduced[chan] + diff;

        if (val_reduite < 0)
            val_reduite = 0;
        if (val_reduite > 127)
            val_reduite = 127;

        prev_reduced[chan] = val_reduite;
        img_out[i] = (unsigned char)(val_reduite * 2);
    }

    fwrite(img_out, 1, num_pixels, f_out);

    free(compressed_data);
    free(img_out);
    fclose(f_in);
    fclose(f_out);

    return 0;
}
