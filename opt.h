#ifndef OPT_H
#define OPT_H
#include <stdbool.h>

typedef struct {
    char *filename; 
    bool tokens;     
    bool symtab;     
    bool trace;    
} Options;


void opts_parse(int argc, char *argv[]);

Options *opts_get(void);

#endif
