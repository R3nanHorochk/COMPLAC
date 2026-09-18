#include "symtab.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 2048
#define MAX_SCOPE_DEPTH 64

static SymEntry entries[MAX_ENTRIES];
static int total_entries = 0;

static char scope_stack[MAX_SCOPE_DEPTH][128];
static int scope_levels[MAX_ENTRIES];
static int current_level = -1;

const char* symtab_cat_to_str(SymCategory cat) {
    switch (cat) {
        case SYM_VAR_GLOBAL: return "var_global";
        case SYM_VAR_LOCAL:  return "var_local";
        case SYM_PARAM:      return "param";
        case SYM_PROC:       return "proc";
        case SYM_FUNC:       return "func";
        default:             return "desconhecido";
    }
}

void symtab_init(void) {
    total_entries = 0;
    current_level = -1;
    symtab_enter_scope("global");
}

void symtab_enter_scope(const char *scope_name) {
    if (current_level >= MAX_SCOPE_DEPTH - 1) {
        diag_error(0, "Limite maximo de profundidade de escopo atingido.");
    }
    current_level++;
    strncpy(scope_stack[current_level], scope_name, sizeof(scope_stack[current_level]) - 1);
    scope_stack[current_level][sizeof(scope_stack[current_level]) - 1] = '\0';
}

void symtab_leave_scope(void) {
    if (current_level > 0) {
        current_level--;
    }
}

const char* symtab_get_current_scope_name(void) {
    if (current_level >= 0) {
        return scope_stack[current_level];
    }
    return "global";
}

int symtab_insert(const char *id, SymCategory cat, const char *type, int extra) {
    /* Verifica duplicidade de declaracao no mesmo nivel de escopo */
    for (int i = 0; i < total_entries; i++) {
        if (scope_levels[i] == current_level && strcmp(entries[i].id, id) == 0) {
            return 0; // Erro: identificador ja declarado neste escopo
        }
    }

    if (total_entries >= MAX_ENTRIES) {
        diag_error(0, "Tabela de simbolos esgotada.");
    }

    strncpy(entries[total_entries].id, id, sizeof(entries[total_entries].id) - 1);
    entries[total_entries].id[sizeof(entries[total_entries].id) - 1] = '\0';

    strncpy(entries[total_entries].scope, scope_stack[current_level], sizeof(entries[total_entries].scope) - 1);
    entries[total_entries].scope[sizeof(entries[total_entries].scope) - 1] = '\0';

    entries[total_entries].cat = cat;

    strncpy(entries[total_entries].type, type, sizeof(entries[total_entries].type) - 1);
    entries[total_entries].type[sizeof(entries[total_entries].type) - 1] = '\0';

    entries[total_entries].extra = extra;
    scope_levels[total_entries] = current_level;

    total_entries++;
    return 1;
}

int symtab_lookup(const char *id) {
    /* Procura do escopo mais interno ao mais externo */
    for (int i = total_entries - 1; i >= 0; i--) {
        if (strcmp(entries[i].id, id) == 0) {
            return 1;
        }
    }
    return 0;
}

int symtab_get_count(void) {
    return total_entries;
}

const SymEntry* symtab_get_entry(int index) {
    if (index >= 0 && index < total_entries) {
        return &entries[index];
    }
    return NULL;
}

void symtab_destroy(void) {
    total_entries = 0;
    current_level = -1;
}