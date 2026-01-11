#include "../../Codec/include/dif.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int convert_to_ppm(const char *input_file, const char *output_file) {
    char command[1024];
    sprintf(command, "convert \"%s\" \"%s\"", input_file, output_file);

    printf(">> Conversion automatique : %s -> %s\n", input_file, output_file);
    int ret = system(command);

    if (ret != 0) {
        fprintf(stderr, "Erreur\n");
    }
    return ret;
}

long get_file_size(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size;
}

static void launch_viewer(const char *program, const char *filename) {
    if (!program)
        return;
    char command[1024];
    sprintf(command, "%s \"%s\" &", program, filename);
    system(command);
}

static int read_token(FILE *f, char *buf, size_t max_len) {
    int c;
    do {
        c = fgetc(f);
        if (c == '#') {
            while (c != '\n' && c != EOF)
                c = fgetc(f);
        }
    } while (c != EOF && isspace((unsigned char)c));

    if (c == EOF)
        return 1;

    size_t i = 0;
    while (c != EOF && !isspace((unsigned char)c)) {
        if (i + 1 < max_len)
            buf[i++] = (char)c;
        c = fgetc(f);
    }
    buf[i] = '\0';
    return 0;
}

long get_raw_pnm_size(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return 0;

    char token[64];

    if (read_token(f, token, sizeof(token))) {
        fclose(f);
        return 0;
    }
    int channels = 0;
    if (strcmp(token, "P5") == 0)
        channels = 1;
    else if (strcmp(token, "P6") == 0)
        channels = 3;
    else {
        fclose(f);
        return 0;
    }

    if (read_token(f, token, sizeof(token))) {
        fclose(f);
        return 0;
    }
    long width = strtol(token, NULL, 10);

    if (read_token(f, token, sizeof(token))) {
        fclose(f);
        return 0;
    }
    long height = strtol(token, NULL, 10);

    if (read_token(f, token, sizeof(token))) {
        fclose(f);
        return 0;
    }
    long maxval = strtol(token, NULL, 10);

    fclose(f);

    if (width <= 0 || height <= 0 || maxval != 255)
        return 0;
    return width * height * channels;
}

void print_help(const char *prog_name) {
    printf("Usage: %s [options] <file1> [file2 ...]\n", prog_name);
    printf("Options:\n");
    printf("  -h          Aide\n");
    printf("  -v          Mode verbeux\n");
    printf("  -t          Chronomètre\n");
    printf("  -x <prog>   Ouvrir avec un visualiseur après décodage (ex: -x "
           "eog)\n");
}

static int process_file(const char *input, int verbose, int timer,
                        const char *viewer) {
    char output[1024];
    char temp_ppm[1024] = "";

    char *actual_source_file = (char *)input;
    int needs_cleanup = 0;
    int is_decoding = 0;
    long ref_raw_size = 0;

    char *ext = strrchr(input, '.');

    if (ext && strcmp(ext, ".dif") == 0) {
        is_decoding = 1;

        strncpy(output, input, ext - input);
        output[ext - input] = '\0';
        strcat(output, ".pnm");

        ref_raw_size = get_file_size(input);

    } else {
        is_decoding = 0;

        int is_native =
            (ext && (strcmp(ext, ".ppm") == 0 || strcmp(ext, ".pgm") == 0));

        if (!is_native) {
            sprintf(temp_ppm, "temp_%lx.ppm",
                    (unsigned long)clock()); // Unique name

            if (convert_to_ppm(input, temp_ppm) != 0)
                return 1;

            actual_source_file = temp_ppm;
            needs_cleanup = 1;
        }

        ref_raw_size = get_raw_pnm_size(actual_source_file);

        if (ext) {
            strncpy(output, input, ext - input);
            output[ext - input] = '\0';
        } else {
            strcpy(output, input);
        }
        strcat(output, ".dif");
    }

    if (verbose) {
        printf("Mode   : %s\n", is_decoding ? "DÉCOMPRESSION" : "COMPRESSION");
        printf("Entrée : %s\nSortie : %s\n", input, output);
    }

    clock_t start = clock();
    int lib_ret = 0;

    if (is_decoding) {
        lib_ret = diftopnm(actual_source_file, output);
    } else {
        lib_ret = pnmtodif(actual_source_file, output);
    }

    clock_t end = clock();
    double duration = ((double)(end - start)) / CLOCKS_PER_SEC;

    if (lib_ret != 0) {
        fprintf(stderr, "Echec sur %s (Code %d)\n", input, lib_ret);
        if (needs_cleanup)
            remove(temp_ppm);
        return lib_ret;
    }

    if (verbose || timer) {
        printf("--------------------------------\n");
        if (timer)
            printf("Temps : %.4f sec\n", duration);

        long final_size = get_file_size(output);

        if (ref_raw_size > 0) {
            if (!is_decoding) {
                float ratio = (float)final_size / ref_raw_size * 100.0f;
                printf("Taille BRUTE : %ld octets\n", ref_raw_size);
                printf("Taille DIF            : %ld octets\n", final_size);
                printf("RATIO                 : %.2f%% (%.2f%% gain)\n", ratio,
                       100.0f - ratio);
            } else {
                printf("Taille DIF (Entrée)   : %ld octets\n", ref_raw_size);
                printf("Taille PNM (Sortie)   : %ld octets\n", final_size);
            }
        }
        printf("Succès.\n\n");
    }

    if (is_decoding && viewer) {
        if (verbose)
            printf("Ouverture avec %s...\n", viewer);
        launch_viewer(viewer, output);
    }

    if (needs_cleanup) {
        remove(temp_ppm);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    int verbose = 0;
    int timer = 0;
    const char *viewer = NULL;

    const char *file_list[512];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            timer = 1;
        } else if (strcmp(argv[i], "-x") == 0) {
            if (i + 1 < argc)
                viewer = argv[++i];
            else {
                fprintf(stderr, "Erreur: -x nécessite un nom de programme.\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            if (file_count < 512)
                file_list[file_count++] = argv[i];
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "Erreur: Aucun fichier spécifié.\n");
        return 1;
    }

    int global_error = 0;

    for (int k = 0; k < file_count; k++) {
        int ret = process_file(file_list[k], verbose, timer, viewer);
        if (ret != 0)
            global_error = ret;
    }

    return global_error;
}
