#include "lex.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static FILE *source_file = NULL;
static int char_atual = ' ';
static int linha_atual = 1;

typedef struct {
    const char *kw;
    TokenCategoria cat;
} Reservada;

static const Reservada Reservadas[] = {
    {"globvars",  sGLOBVARS},
    {"locvars",   sLOCVARS},
    {"is",        sKIND},
    {"int",       sINT},
    {"logic",     sLOGIC},
    {"chr",       sCHR},
    {"func",      sFUNC},
    {"proc",      sPROC},
    {"ref",       sREF},
    {"start",     sSTART},
    {"end",       sEND},
    {"echo",      sECHO},
    {"get",       sGET},
    {"case",      sCASE},
    {"otherwise", sOTHERWISE},
    {"choose",    sCHOOSE},
    {"match",     sMATCH},
    {"others",    sOTHERS},
    {"for",       sFOR},
    {"from",      sFROM},
    {"to",        sTO},
    {"by",        sBY},
    {"do",        sDO},
    {"while",     sWHILE},
    {"repeat",    sREPEAT},
    {"until",     sUNTIL},
    {"return",    sRETURN}
};

static const int TotalReservadas = sizeof(Reservadas) / sizeof(Reservadas[0]);

/* Avança a leitura em um caractere e atualiza a contagem de linhas */
static void next_char(void) {
    char_atual = fgetc(source_file);
    if (char_atual == '\n') {
        linha_atual++;
    }
}

void lex_init(const char *filename) {
    source_file = fopen(filename, "r");
    if (!source_file) {
        diag_error(0, "Nao foi possivel abrir o ficheiro de entrada.");
    }
    linha_atual = 1;
    next_char();
}

void lex_close(void) {
    if (source_file) {
        fclose(source_file);
        source_file = NULL;
    }
}

static TokenCategoria CheckReservada(const char *text) {
    for (int i = 0; i < TotalReservadas; i++) {
        if (strcmp(text, Reservadas[i].kw) == 0) {
            return Reservadas[i].cat;
        }
    }
    return sIDENTIF;
}

Token lex_next(void) {
    Token token;
    token.cat = TOKEN_ERRO;
    token.lexeme[0] = '\0';

    while (char_atual != EOF) {
        if (isspace(char_atual)) {
            next_char();
            continue;
        }

        if (char_atual == '#') {
            next_char();
            while (char_atual != '\n' && char_atual != EOF) {
                next_char();
            }
            continue;
        }

        if (char_atual == '/') {
            int inicio_linha = linha_atual;
            next_char();

            if (char_atual == '#') {
                next_char();
                while (1) {
                    if (char_atual == EOF) {
                        diag_error_lex(inicio_linha, "Comentario de bloco nao terminado.");
                    }
                    if (char_atual == '#') {
                        next_char();
                        if (char_atual == '/') {
                            next_char();
                            break;
                        }
                    } else {
                        next_char();
                    }
                }
                continue;
            } else if (char_atual == '/') {
                token.line = inicio_linha;
                token.cat = sDIV;
                token.lexeme[0] = '\0';
                next_char();
                return token;
            } else {
                diag_error_lex(inicio_linha, "Caractere inesperado apos barra.");
            }
        }

        break;
    }

    token.line = linha_atual;

    if (char_atual == EOF) {
        token.cat = TOKEN_EOF;
        token.lexeme[0] = '\0';
        return token;
    }

    /* 1. Cadeia de caracteres literal: "..." -> armazena o lexema[cite: 1, 5, 6] */
    if (char_atual == '"') {
        next_char();
        int len = 0;
        while (char_atual != EOF && char_atual != '"' && char_atual != '\n') {
            if (len < 255) {
                token.lexeme[len++] = (char)char_atual;
            }
            next_char();
        }
        token.lexeme[len] = '\0';

        if (char_atual == '"') {
            next_char();
            token.cat = sSTRING;
        } else {
            diag_error_lex(token.line, "Cadeia de caracteres (string) nao terminada.");
        }
        return token;
    }

    if (char_atual == '\'') {
        next_char();
        if (char_atual == EOF || char_atual == '\'' || char_atual == '\n') {
            diag_error_lex(token.line, "Constante caractere vazia ou invalida.");
        }
        token.lexeme[0] = (char)char_atual;
        token.lexeme[1] = '\0';
        next_char();

        if (char_atual != '\'') {
            diag_error_lex(token.line, "Constante caractere nao terminada com aspa simples.");
        }
        next_char();
        token.cat = sCTECHAR;
        return token;
    }

    if (isalpha(char_atual) || char_atual == '_') {
        char buffer[256];
        int len = 0;
        while (isalnum(char_atual) || char_atual == '_') {
            if (len < 255) {
                buffer[len++] = (char)char_atual;
            }
            next_char();
        }
        buffer[len] = '\0';

        TokenCategoria cat_encontrada = CheckReservada(buffer);
        if (cat_encontrada == sIDENTIF) {
            token.cat = sIDENTIF;
            strncpy(token.lexeme, buffer, 255);
            token.lexeme[255] = '\0';
        } else {
            token.cat = cat_encontrada;
            token.lexeme[0] = '\0';
        }
        return token;
    }

    if (isdigit(char_atual)) {
        int len = 0;
        while (isdigit(char_atual)) {
            if (len < 255) {
                token.lexeme[len++] = (char)char_atual;
            }
            next_char();
        }
        token.lexeme[len] = '\0';
        token.cat = sCTEINT;
        return token;
    }

    char ch = (char)char_atual;
    next_char();

    switch (ch) {
        case '<':
            if (char_atual == '<') { next_char(); token.cat = sATRIB; }
            else if (char_atual == '=') { next_char(); token.cat = sMENORIGUAL; }
            else { token.cat = sMENOR; }
            return token;

        case '>':
            if (char_atual == '=') { next_char(); token.cat = sMAIORIGUAL; }
            else { token.cat = sMAIOR; }
            return token;

        case '-':
            if (char_atual == '>') { next_char(); token.cat = sIMPLIC; }
            else { token.cat = sSUBRAT; }
            return token;

        case '~':
            if (char_atual == '=') { next_char(); token.cat = sDIFERENTE; }
            else { token.cat = sNEG; }
            return token;

        case '=': token.cat = sIGUAL;      return token;
        case '+': token.cat = sSOMA;       return token;
        case '*': token.cat = sMULT;       return token;
        case '&': token.cat = sAND;        return token;
        case '|': token.cat = sOR;         return token;
        case '(': token.cat = sABREPAR;    return token;
        case ')': token.cat = sFECHAPAR;   return token;
        case '[': token.cat = sABRECOL;    return token;
        case ']': token.cat = sFECHACOL;   return token;
        case ';': token.cat = sPONTOVIRG;  return token;
        case ',': token.cat = sVIRG;       return token;
        case ':': token.cat = sDOISPONTOS; return token;

        default:
            diag_error_lex(token.line, "Caractere desconhecido ou invalido.");
            return token;
    }
}