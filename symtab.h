#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdbool.h>

typedef enum {
    CAT_VAR_GLOBAL,
    CAT_VAR_LOCAL,
    CAT_PARAM,
    CAT_PROCEDIMENTO,
    CAT_FUNCAO
} SimboloCat;

typedef enum {
    TIPO_INT,
    TIPO_BOOL,
    TIPO_CHR,
    TIPO_VOID
} SimboloTipo;

typedef struct {
    bool ocupado;           // Indica se a posição da tabela está em uso
    char id[32];
    SimboloCat cat;
    SimboloTipo tipo;
    int extra;
    char scope[64];
    int level;              // Nível de escopo (0 = global)
} Simbolo;

void symtab_init(void);
void symtab_destroy(void);

void symtab_enter_scope(const char *name);
void symtab_leave_scope(void);

bool symtab_insert(const char *id, SimboloCat cat, SimboloTipo tipo, int extra);
Simbolo *symtab_lookup(const char *id);

void symtab_print(void);

#endif