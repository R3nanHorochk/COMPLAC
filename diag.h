#ifndef DIAG_H
#define DIAG_H

/* Inicializa o modulo de diagnostico com o nome do ficheiro-fonte */
void diag_init(char *source_filename);

/* Reporta uma mensagem informativa ou de rastreamento (--trace) */
void diag_info(char *msg);

/* Reporta erro lexico com indicacao de linha e encerra a execucao */
void diag_error_lex(int line, char *msg);

/* Reporta erro sintatico indicando linha, token esperado e encontrado */
void diag_error_syntax(int line, char *expected, char *found);

/* Reporta erro generico com linha e encerra a execucao */
void diag_error(int line, char *msg);

#endif /* DIAG_H */