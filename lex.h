#ifndef LEX_H
#define LEX_H

#include "token.h"

void lex_init(const char *source_filename);
Token lex_next(void);
void lex_close(void);

#endif 