#ifndef TOKEN_H
#define TOKEN_H

/* Categorias de tokens conforme a gramatica formal EBNF da SLAC2 */
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    /* Categorias basicas e literais */
    sIDENTIF,       /* Identificadores */
    sCTEINT,        /* Constantes inteiras */
    sCTECHAR,       /* Caracteres literais ('a') */
    sSTRING,        /* Cadeias de caracteres ("texto") */

    /* Palavras reservadas */
    sGLOBVARS,      /* globvars */
    sLOCVARS,       /* locvars */
    sKIND,          /* is */
    sINT,           /* int */
    sLOGIC,         /* logic */
    sCHR,           /* chr */
    sFUNC,          /* func */
    sPROC,          /* proc */
    sREF,           /* ref */
    sSTART,         /* start */
    sEND,           /* end */
    sECHO,          /* echo */
    sGET,           /* get */
    sCASE,          /* case */
    sOTHERWISE,     /* otherwise */
    sCHOOSE,        /* choose */
    sMATCH,         /* match */
    sOTHERS,        /* others */
    sFOR,           /* for */
    sFROM,          /* from */
    sTO,            /* to */
    sBY,            /* by */
    sDO,            /* do */
    sWHILE,         /* while */
    sREPEAT,        /* repeat */
    sUNTIL,         /* until */
    sRETURN,        /* return */

    /* Operadores e delimitadores */
    sATRIB,         /* << */
    sIMPLIC,        /* -> */
    sSOMA,          /* + */
    sSUBRAT,        /* - */
    sMULT,          /* * */
    sDIV,           /* // */
    sMAIOR,         /* > */
    sMAIORIGUAL,    /* >= */
    sMENOR,         /* < */
    sMENORIGUAL,    /* <= */
    sIGUAL,         /* = */
    sDIFERENTE,     /* ~= */
    sAND,           /* & */
    sOR,            /* | */
    sNEG,           /* ~ */
    sABREPAR,       /* ( */
    sFECHAPAR,      /* ) */
    sABRECOL,       /* [ */
    sFECHACOL,      /* ] */
    sPONTOVIRG,     /* ; */
    sVIRG,          /* , */
    sDOISPONTOS     /* : */
} TokenCategoria;

/* Representacao de uma ocorrencia de token no codigo-fonte */
typedef struct {
    TokenCategoria cat;
    char lexeme[256];
    int line;
} Token;

/* Retorna o nome textual exato da categoria para uso em relatorios e logs */
const char *token_cat_name(TokenCategoria cat);

#endif /* TOKEN_H */