#include "diag.h"
#include <stdio.h>
#include <stdlib.h>

char *filename = "file.slac";


void diag_init(char *source_filename) {
    if (source_filename != NULL) {
        strncpy(filename, source_filename, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
}

void diag_info(char *msg) {
    printf("INFO: %s\n", msg);
}

void diag_error_lex(int linha, char *msg) {
    printf("\nERRO LEXICO: %s:%d: %s\n", filename, linha, msg);
    exit(1);
}

void diag_error_sintat(int linha, char *expected, char *found) {
    printf("\nERRO SINTATICO: %s:%d: Esperado '%s', mas encontrado '%s'\n", filename, linha, expected, found);
    exit(1);
}

void diag_error(int linha, char *msg) {
    if (linha > 0) {
        printf("\nERRO: %s:%d: %s\n", filename, linha, msg);
    } else {
        printf("\nERRO: %s: %s\n", filename, msg);
    }
    exit(1);
}