#include "symtab.h"
#include <stdio.h>
#include <string.h>

#define M 250 // Tamanho fixo da tabela

static Simbolo tabela[M];
static char escopo_atual[64] = "global";
static int nivel_atual = 0;

static int hash_func(const char *id) {
    int k = 0;
    while (*id) {
        k += (char)(*id);
        id++;
    }
    return k % M;
}

void symtab_init(void) {
    for (int i = 0; i < M; i++) {
        tabela[i].ocupado = false;
    }
    strcpy(escopo_atual, "global");
    nivel_atual = 0;
}

void symtab_enter_scope(const char *name) {
    nivel_atual++;
    strncpy(escopo_atual, name, sizeof(escopo_atual) - 1);
    escopo_atual[sizeof(escopo_atual) - 1] = '\0';
}

void symtab_leave_scope(void) {
    if (nivel_atual <= 0) return;

    for (int i = 0; i < M; i++) {
        if (tabela[i].ocupado && tabela[i].level == nivel_atual) {
            tabela[i].ocupado = false;
        }
    }

    nivel_atual--;
    if (nivel_atual == 0) {
        strcpy(escopo_atual, "global");
    }
}

bool symtab_insert(const char *id, SimboloCat cat, SimboloTipo tipo, int extra) {
    int start_idx = hash_func(id);
    int idx = start_idx;
    int pos_livre = -1;

    for (int i = 0; i < M; i++) {
        if (tabela[idx].ocupado) {
            if (tabela[idx].level == nivel_atual && strcmp(tabela[idx].id, id) == 0) {
                return false;
            }
        } else {
            if (pos_livre == -1) {
                pos_livre = idx;
                break; 
            }
        }
        idx = (idx + 1) % M; 
    }

    if (pos_livre == -1) {
        return false; 
    }

    tabela[pos_livre].ocupado = true;
    strncpy(tabela[pos_livre].id, id, sizeof(tabela[pos_livre].id) - 1);
    tabela[pos_livre].id[sizeof(tabela[pos_livre].id) - 1] = '\0';
    tabela[pos_livre].cat = cat;
    tabela[pos_livre].tipo = tipo;
    tabela[pos_livre].extra = extra;
    tabela[pos_livre].level = nivel_atual;
    strcpy(tabela[pos_livre].scope, escopo_atual);

    return true;
}


Simbolo *symtab_lookup(char *id) {
    int start_idx = hash_func(id);
    int idx = start_idx;
    Simbolo *melhor_candidato = NULL;

    for (int i = 0; i < M; i++) {
        if (tabela[idx].ocupado && strcmp(tabela[idx].id, id) == 0) {
            if (melhor_candidato == NULL || tabela[idx].level > melhor_candidato->level) {
                melhor_candidato = &tabela[idx];
            }
        }
        idx = (idx + 1) % M; 
    }

    return melhor_candidato;
}

void symtab_print(void) {
    const char *cat_nomes[] = {"var_global", "var_local", "param", "proc", "func"};
    const char *tipo_nomes[] = {"int", "bool", "chr", "void"};

    printf("\n--- TABELA DE SIMBOLOS (Tamanho: %d) ---\n", M);
    for (int i = 0; i < M; i++) {
        if (tabela[i].ocupado) {
            printf("[%03d] Escopo: %-10s | Nivel: %d | ID: %-8s | Cat: %-10s | Tipo: %-5s | Extra: %d\n",
                   i, tabela[i].scope, tabela[i].level, tabela[i].id,
                   cat_nomes[tabela[i].cat], tipo_nomes[tabela[i].tipo], tabela[i].extra);
        }
    }
    printf("----------------------------------------\n");
}

void symtab_destroy(void) {
    for (int i = 0; i < M; i++) {
        tabela[i].ocupado = false;
    }
}