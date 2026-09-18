#ifndef DIAG_H
#define DIAG_H

/* Inicializa o modulo de diagnostico com o nome do ficheiro-fonte */
void diag_init(const char *source_filename);

/* Reporta uma mensagem informativa ou de rastreamento (--trace) */
void diag_info(const char *msg);

/* Reporta erro lexico com indicacao de linha e encerra a execucao */
void diag_error_lex(int line, const char *msg);

/* Reporta erro sintatico indicando linha, token esperado e encontrado */
void diag_error_syntax(int line, const char *expected, const char *found);

/* Reporta erro generico com linha e encerra a execucao */
void diag_error(int line, const char *msg);

#endif /* DIAG_H */