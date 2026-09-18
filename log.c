#include "log.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *file_tk = NULL;
static FILE *file_ts = NULL;
static FILE *file_trc = NULL;

/* Funcao auxiliar para construir nomes de saida trocando a extensao */
static void build_output_filename(const char *src, const char *new_ext, char *dest, size_t max_size) {
    strncpy(dest, src, max_size - 1);
    dest[max_size - 1] = '\0';

    char *dot = strrchr(dest, '.');
    if (dot != NULL) {
        *dot = '\0'; // Remove a extensao antiga (.slac)
    }
    strncat(dest, new_ext, max_size - strlen(dest) - 1);
}

void log_init(const char *source_path, int gen_tk, int gen_ts, int gen_trc) {
    char out_filename[512];

    if (gen_tk) {
        build_output_filename(source_path, ".tk", out_filename, sizeof(out_filename));
        file_tk = fopen(out_filename, "w");
        if (!file_tk) {
            diag_error(0, "Falha ao criar o arquivo de tokens (.tk).");
        }
    }

    if (gen_ts) {
        build_output_filename(source_path, ".ts", out_filename, sizeof(out_filename));
        file_ts = fopen(out_filename, "w");
        if (!file_ts) {
            diag_error(0, "Falha ao criar o arquivo da tabela de simbolos (.ts).");
        }
    }

    if (gen_trc) {
        build_output_filename(source_path, ".trc", out_filename, sizeof(out_filename));
        file_trc = fopen(out_filename, "w");
        if (!file_trc) {
            diag_error(0, "Falha ao criar o arquivo de rastreamento (.trc).");
        }
    }
}

void log_token(const Token *t) {
    if (file_tk && t) {
        // Formato oficial exigido: <L> <CAT> "<LEX>"
        fprintf(file_tk, "%d %s \"%s\"\n", t->line, token_cat_name(t->cat), t->lexeme);
    }
}

void log_symtab_entry(const char *scope, const char *id, const char *cat, const char *tipo, int extra) {
    if (file_ts) {
        // Formato oficial exigido: SCOPE=<descr> id="<lexema>" cat=<categ> tipo=<tipo> extra=<atrib>
        fprintf(file_ts, "SCOPE=%s id=\"%s\" cat=%s tipo=%s extra=%d\n",
                scope, id, cat, tipo, extra);
    }
}

void log_trace(const char *msg) {
    if (file_trc) {
        fprintf(file_trc, "%s\n", msg);
    }
}

void log_close(void) {
    if (file_tk) {
        fclose(file_tk);
        file_tk = NULL;
    }
    if (file_ts) {
        fclose(file_ts);
        file_ts = NULL;
    }
    if (file_trc) {
        fclose(file_trc);
        file_trc = NULL;
    }
}