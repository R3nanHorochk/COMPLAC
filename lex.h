#ifndef LEX_H
#define LEX_H

#include "token.h"

/* Inicializa o analisador lexico com o ficheiro-fonte */
void lex_init(const char *filename);

/* Obtem o proximo token da entrada de forma sequencial */
Token lex_next(void);

/* Fecha o ficheiro-fonte aberto */
void lex_close(void);

#endif /* LEX_H */