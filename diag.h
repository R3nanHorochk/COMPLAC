#ifndef DIAG_H
#define DIAG_H

void diag_init(char *source_filename);
void diag_info(char *msg);
void diag_error_lex(int line, char *msg);
void diag_error_syntax(int line, char *expected, char *found);
void diag_error(int line, char *msg);

#endif 