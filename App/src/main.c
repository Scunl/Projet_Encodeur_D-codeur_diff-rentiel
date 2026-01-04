#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../Codec/include/dif.h"

long get_file_size(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size;
}

void print_help(const char *prog_name) {
    printf("Usage: %s [options] <fichier>\n", prog_name);
    printf("Options:\n");
    printf("  -h      Affiche cette aide\n");
    printf("  -v      Mode verbeux (détails sur la console)\n");
    printf("  -t      Mesure du temps d'exécution\n");
    printf("Description:\n");
    printf("  Si <fichier> est .dif -> Décompression vers .pnm\n");
    printf("  Sinon (ex: .pgm, .ppm) -> Compression vers .dif\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    int verbose = 0;
    int timer = 0;
    char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            timer = 1;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Erreur fichier\n");
        return 1;
    }

    char output_file[256];
    int is_decoding = 0;
    char *ext = strrchr(input_file, '.');

    if (ext && strcmp(ext, ".dif") == 0) {
        is_decoding = 1;
        strncpy(output_file, input_file, ext - input_file);
        output_file[ext - input_file] = '\0';
        strcat(output_file, ".pnm");
    } else {
        if (ext) {
            strncpy(output_file, input_file, ext - input_file);
            output_file[ext - input_file] = '\0';
        } else {
            strcpy(output_file, input_file);
        }
        strcat(output_file, ".dif");
    }

    if (verbose) {
        printf("Mode: %s\n", is_decoding ? "DÉCOMPRESSION" : "COMPRESSION");
        printf("Entrée: %s\nSortie: %s\n", input_file, output_file);
    }

    clock_t start = clock();
    

    if (is_decoding) {
        diftopnm(input_file, output_file);
    } else {
        pnmtodif(input_file, output_file);
    }

    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    

    if (verbose || timer) {
        printf("--------------------------------\n");
        if (timer) printf("Temps d'exécution : %.4f sec\n", time_taken);
        
        if (!is_decoding) {
            long size_in = get_file_size(input_file);
            long size_out = get_file_size(output_file);
            if (size_in > 0) {
                float ratio = (float)size_out / size_in * 100.0;
                printf("Taille originale : %ld octets\n", size_in);
                printf("Taille compressée: %ld octets\n", size_out);
                printf("Ratio            : %.2f%% (%.2f%% gain)\n", ratio, 100.0 - ratio);
            }
        }
        printf("Succès.\n");
    }

    return 0;
}