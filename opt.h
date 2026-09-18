#ifndef OPT_H
#define OPT_H

/* Estrutura que armazena os parametros recebidos via linha de comando */
typedef struct {
    char source_path[512];
    int gen_tokens;   /* 1 se --tokens foi fornecido, caso contrario 0 */
    int gen_symtab;   /* 1 se --symtab foi fornecido, caso contrario 0 */
    int gen_trace;    /* 1 se --trace foi fornecido, caso contrario 0 */
} CompilerOptions;

/* Processa os argumentos da CLI (argc, argv) e inicializa as configuracoes */
void opts_parse(int argc, char *argv[]);

/* Retorna a estrutura com as opcoes ativas para uso geral do compilador */
const CompilerOptions* opts_get(void);

#endif /* OPT_H */