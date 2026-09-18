#include "opt.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CompilerOptions options;

void opts_parse(int argc, char *argv[]) {
    if (argc < 2) {
        diag_error(0, "Uso: complac <arquivo.slac> [--tokens | --symtab | --trace]");
    }

    // Inicializa valores padrao
    options.source_path[0] = '\0';
    options.gen_tokens = 0;
    options.gen_symtab = 0;
    options.gen_trace = 0;

    int source_defined = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) {
            options.gen_tokens = 1;
        } else if (strcmp(argv[i], "--symtab") == 0) {
            options.gen_symtab = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            options.gen_trace = 1;
        } else if (argv[i][0] == '-') {
            diag_error(0, "Opcao de linha de comando desconhecida.");
        } else {
            if (!source_defined) {
                strncpy(options.source_path, argv[i], sizeof(options.source_path) - 1);
                options.source_path[sizeof(options.source_path) - 1] = '\0';
                source_defined = 1;
            } else {
                diag_error(0, "Apenas um arquivo-fonte deve ser informado.");
            }
        }
    }

    if (!source_defined) {
        diag_error(0, "Nenhum arquivo-fonte .slac foi especificado.");
    }
}

const CompilerOptions* opts_get(void) {
    return &options;
}