#include "diag.h"
#include <stdio.h>
#include <stdlib.h>

static const char *current_filename = "fonte";

void diag_init(const char *source_filename) {
    if (source_filename != NULL) {
        current_filename = source_filename;
    }
}

void diag_info(const char *msg) {
    printf("[INFO] %s\n", msg);
}

void diag_error_lex(int line, const char *msg) {
    fprintf(stderr, "\n[ERRO LEXICO] %s:%d: %s\n", current_filename, line, msg);
    exit(EXIT_FAILURE);
}

void diag_error_syntax(int line, const char *expected, const char *found) {
    fprintf(stderr, "\n[ERRO SINTATICO] %s:%d: Esperado '%s', mas encontrado '%s'\n",
            current_filename, line, expected, found);
    exit(EXIT_FAILURE);
}

void diag_error(int line, const char *msg) {
    if (line > 0) {
        fprintf(stderr, "\n[ERRO] %s:%d: %s\n", current_filename, line, msg);
    } else {
        fprintf(stderr, "\n[ERRO] %s: %s\n", current_filename, msg);
    }
    exit(EXIT_FAILURE);
}