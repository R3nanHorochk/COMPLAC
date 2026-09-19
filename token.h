#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_EOF,
    TOKEN_ERRO,
    sIDENTIF,
    sCTEINT,
    sCTECHAR,
    sSTRING,
    sGLOBVARS,
    sLOCVARS,
    sKIND,
    sINT,
    sLOGIC,
    sCHR,
    sFUNC,
    sPROC,
    sREF,
    sSTART,
    sEND,
    sECHO,
    sGET,
    sCASE,
    sOTHERWISE,
    sCHOOSE,
    sMATCH,
    sOTHERS,
    sFOR,
    sFROM,
    sTO,
    sBY,
    sDO,
    sWHILE,
    sREPEAT,
    sUNTIL,
    sRETURN,
    sATRIB,
    sIMPLIC,
    sSOMA,
    sSUBRAT,
    sMULT,
    sDIV,
    sMAIOR,
    sMAIORIGUAL,
    sMENOR,
    sMENORIGUAL,
    sIGUAL,
    sDIFERENTE,
    sAND,
    sOR,
    sNEG,
    sABREPAR,
    sFECHAPAR,
    sABRECOL,
    sFECHACOL,
    sPONTOVIRG,
    sVIRG,
    sDOISPONTOS
} TokenCategoria;

typedef struct {
    TokenCategoria cat;
    char lexeme[256];
    int line;
} Token;

const char *tokenCatNome(TokenCategoria cat);

#endif 