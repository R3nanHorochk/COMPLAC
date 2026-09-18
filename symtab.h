#ifndef SYMTAB_H
#define SYMTAB_H

/* Categorias de simbolos segundo a SLAC2 */
typedef enum {
    SYM_VAR_GLOBAL,
    SYM_VAR_LOCAL,
    SYM_PARAM,
    SYM_PROC,
    SYM_FUNC
} SymCategory;

/* Estrutura publica de uma entrada da Tabela de Simbolos */
typedef struct {
    char id[64];
    char scope[128];
    SymCategory cat;
    char type[64];
    int extra;
} SymEntry;

/* Inicializa a Tabela de Simbolos e define o escopo global inicial */
void symtab_init(void);

/* Empilha um novo escopo (ex: sub-rotina ou bloco interno start...end) */
void symtab_enter_scope(const char *scope_name);

/* Desempilha o escopo corrente e regressa ao escopo pai */
void symtab_leave_scope(void);

/* Retorna o caminho descritivo do escopo atual (ex: "global", "fn:SOMA.locals", "proc:main.block#1") */
const char* symtab_get_current_scope_name(void);

/* Insere identificador no escopo atual. Devolve 1 se sucesso, 0 se ja declarado no mesmo escopo */
int symtab_insert(const char *id, SymCategory cat, const char *type, int extra);

/* Procura retroativamente um identificador nos escopos visiveis */
int symtab_lookup(const char *id);

/* Retorna a quantidade total de entradas gravadas na TS */
int symtab_get_count(void);

/* Retorna uma entrada especifica pelo indice para geracao de relatorios pelo modulo log */
const SymEntry* symtab_get_entry(int index);

/* Converte a categoria de simbolo em texto para formatacao de log */
const char* symtab_cat_to_str(SymCategory cat);

/* Liberta as estruturas internas */
void symtab_destroy(void);

#endif /* SYMTAB_H */