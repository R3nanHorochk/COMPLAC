#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char filename[512] = "file.slac";

void diag_init(const char *source_filename) {
    if (source_filename != NULL) {
        strncpy(filename, source_filename, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
}

void diag_info(const char *msg) {
    printf("INFO: %s\n", msg);
}

void diag_error_lex(int linha, const char *msg) {
    printf("\nERRO LEXICO: %s:%d: %s\n", filename, linha, msg);
    exit(1);
}

void diag_error_syntax(int linha, const char *expected, const char *found) {
    printf("\nERRO SINTATICO: %s:%d: Esperado '%s', mas encontrado '%s'\n", filename, linha, expected, found);
    exit(1);
}

void diag_error(int linha, const char *msg) {
    if (linha > 0) {
        printf("\nERRO: %s:%d: %s\n", filename, linha, msg);
    } else {
        printf("\nERRO: %s: %s\n", filename, msg);
    }
    exit(1);
}
