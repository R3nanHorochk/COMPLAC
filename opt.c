#include "opt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Options GOpts = {
    .filename = NULL,
    .tokens = false,
    .symtab = false,
    .trace = false
};

void opts_parse(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.slac> [--tokens] [--symtab] [--trace]\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) {
            GOpts.tokens = true;
        } else if (strcmp(argv[i], "--symtab") == 0) {
            GOpts.symtab = true;
        } else if (strcmp(argv[i], "--trace") == 0) {
            GOpts.trace = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Erro: Opcao desconhecida '%s'\n", argv[i]);
            exit(1);
        } else {
            if (GOpts.filename != NULL) {
                fprintf(stderr, "Erro: Mais de um arquivo-fonte fornecido ('%s' e '%s')\n",
                        GOpts.filename, argv[i]);
                exit(1);
            }
            GOpts.filename = argv[i];
        }
    }

    if (GOpts.filename == NULL) {
        fprintf(stderr, "Erro: Nenhum arquivo-fonte informado.\n");
        exit(1);
    }
}

Options *opts_get(void) {
    return &GOpts;
}