#ifndef LOG_H
#define LOG_H

#include "token.h"

/* Inicializa os ficheiros de saida (.tk, .ts, .trc) com base no caminho do fonte */
void log_init(const char *source_path, int gen_tk, int gen_ts, int gen_trc);

/* Registra um token no log .tk com o formato: <L> <CAT> "<LEX>" */
void log_token(const Token *t);

/* Registra uma entrada no log .ts com o formato oficial da especificacao */
void log_symtab_entry(const char *scope, const char *id, const char *cat, const char *tipo, int extra);

/* Registra uma mensagem de rastreamento no arquivo .trc */
void log_trace(const char *msg);

/* Fecha os arquivos de log que foram abertos */
void log_close(void);

#endif /* LOG_H */