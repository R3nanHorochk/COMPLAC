#ifndef DIAG_H
#define DIAG_H

void diag_init(const char *source_filename);
void diag_info(const char *msg);
void diag_error_lex(int line, const char *msg);
void diag_error_syntax(int line, const char *expected, const char *found);
void diag_error(int line, const char *msg);

#endif
