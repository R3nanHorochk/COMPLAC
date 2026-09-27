#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "diag.h"
#include "opt.h"
#include "log.h"
#include "lex.h"
#include "symtab.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    /* 1. Processa argumentos da linha de comando */
    opts_parse(argc, argv);
    const Options *opts = opts_get();

    /* 2. Inicializa o modulo de diagnosticos */
    diag_init(opts->filename);

    /* 3. Inicializa os ficheiros de log opcionais (.tk, .ts, .trc) */
    log_init(opts->filename, opts->tokens, opts->symtab, opts->trace);

    /* 4. Inicializa o analisador lexico com o ficheiro-fonte */
    lex_init(opts->filename);

    /* 5. Inicializa a Tabela de Simbolos */
    symtab_init();

    if (opts->trace) {
        log_trace("Iniciando a analise do compilador COMPLAC...");
    }

    /* 6. Dispara a analise sintatica descendente recursiva (consome lex sob demanda) */
    parse_program();

    /* 7. Se solicitado --symtab, descarrega a tabela de simbolos consolidada */
    if (opts->symtab) {
        int count = symtab_get_count();
        for (int i = 0; i < count; i++) {
            const SymEntry *entry = symtab_get_entry(i);
            if (entry != NULL) {
                log_symtab_entry(entry->scope, entry->id,
                                 symtab_cat_to_str(entry->cat),
                                 symtab_type_to_str(entry->tipo), entry->extra);
            }
        }
    }

    if (opts->trace) {
        log_trace("Compilacao concluida com sucesso.");
    }

    /* 8. Encerramento ordenado de todos os recursos */
    lex_close();
    symtab_destroy();
    log_close();

    return EXIT_SUCCESS;
}
