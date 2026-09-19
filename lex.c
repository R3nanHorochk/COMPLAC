#include "lex.h"
#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE *source_file = NULL;
int char_atual  = ' ';
int linha_atual = 1;

/* Avanca a leitura em um caractere e atualiza a contagem de linhas */
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

/* Tabela de correspondencia para palavras reservadas da linguagem SLAC2 */
static TokenCategoria check_reserved(char *lexeme) {
    if (strcmp(lexeme, "globvars") == 0)   return sGLOBVARS;
    if (strcmp(lexeme, "locvars") == 0)    return sLOCVARS;
    if (strcmp(lexeme, "is") == 0)         return sKIND;
    if (strcmp(lexeme, "int") == 0)        return sINT;
    if (strcmp(lexeme, "logic") == 0)      return sLOGIC;
    if (strcmp(lexeme, "chr") == 0)        return sCHR;
    if (strcmp(lexeme, "func") == 0)       return sFUNC;
    if (strcmp(lexeme, "proc") == 0)       return sPROC;
    if (strcmp(lexeme, "ref") == 0)        return sREF;
    if (strcmp(lexeme, "start") == 0)      return sSTART;
    if (strcmp(lexeme, "end") == 0)        return sEND;
    if (strcmp(lexeme, "echo") == 0)       return sECHO;
    if (strcmp(lexeme, "get") == 0)        return sGET;
    if (strcmp(lexeme, "case") == 0)       return sCASE;
    if (strcmp(lexeme, "otherwise") == 0)  return sOTHERWISE;
    if (strcmp(lexeme, "choose") == 0)     return sCHOOSE;
    if (strcmp(lexeme, "match") == 0)      return sMATCH;
    if (strcmp(lexeme, "others") == 0)     return sOTHERS;
    if (strcmp(lexeme, "for") == 0)        return sFOR;
    if (strcmp(lexeme, "from") == 0)       return sFROM;
    if (strcmp(lexeme, "to") == 0)         return sTO;
    if (strcmp(lexeme, "by") == 0)         return sBY;
    if (strcmp(lexeme, "do") == 0)         return sDO;
    if (strcmp(lexeme, "while") == 0)      return sWHILE;
    if (strcmp(lexeme, "repeat") == 0)     return sREPEAT;
    if (strcmp(lexeme, "until") == 0)      return sUNTIL;
    if (strcmp(lexeme, "return") == 0)     return sRETURN;

    return sIDENTIF;
}

Token lex_next(void) {
    Token token;
    token.lexeme[0] = '\0';

    while (char_atual != EOF) {
        
        if (isspace(char_atual)) {
            next_char();
            continue;
        }

        if (char_atual == '/') {
            int Linha_comeco = linha_atual;
            next_char();

            if (char_atual == '#') {
                next_char();
                while (char_atual != EOF) {
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
                if (char_atual == EOF) {
                    diag_error_lex(Linha_comeco, "Comentario de bloco nao terminado.");
                }
                continue;
            } else if (char_atual == '/') {
                token.cat = sDIV;
                strcpy(token.lexeme, "//");
                token.line = Linha_comeco;
                next_char();
                return token;
            } else {
                diag_error_lex(Linha_comeco, "Caractere inesperado apos '/'. Esperado '/' ou '#'.");
            }
        }

        if (char_atual == '#') {
            while (char_atual != EOF && char_atual != '\n') {
                next_char();
            }
            continue;
        }

        token.line = linha_atual;

        // Identificadores e palavras reservadas: [a-zA-Z][a-zA-Z0-9_]*
        if (isalpha(char_atual)) {
            int len = 0;
            while (isalnum(char_atual) || char_atual == '_') {
                if (len < 255) {
                    token.lexeme[len++] = (char)char_atual;
                }
                next_char();
            }
            token.lexeme[len] = '\0';
            token.cat = check_reserved(token.lexeme);
            return token;
        }

        // Constantes inteiras: [0-9]+
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

        // Cadeias de caracteres (literais de string): "..."
        if (char_atual == '"') {
            int len = 0;
            next_char();
            while (char_atual != EOF && char_atual != '"' && char_atual != '\n') {
                if (char_atual == '\\') {
                    next_char();
                    if (char_atual == '"') {
                        if (len < 255) token.lexeme[len++] = '"';
                        next_char();
                        continue;
                    }
                }
                if (len < 255) {
                    token.lexeme[len++] = (char)char_atual;
                }
                next_char();
            }

            if (char_atual != '"') {
                diag_error_lex(token.line, "Cadeia de caracteres (string) nao terminada.");
            }

            token.lexeme[len] = '\0';
            token.cat = sSTRING;
            next_char();
            return token;
        }

        // Constantes do tipo caractere: 'c'
        if (char_atual == '\'') {
            int len = 0;
            next_char();
            if (char_atual == EOF || char_atual == '\'' || char_atual == '\n') {
                diag_error_lex(token.line, "Constante caractere invalida.");
            }

            if (len < 255) {
                token.lexeme[len++] = (char)char_atual;
            }
            next_char();

            if (char_atual != '\'') {
                diag_error_lex(token.line, "Constante caractere nao terminada com aspa simples.");
            }

            token.lexeme[len] = '\0';
            token.cat = sCTECHAR;
            next_char();
            return token;
        }

        // Operadores compostos ou simples: <, <=, <<
        if (char_atual == '<') {
            next_char();
            if (char_atual == '<') {
                token.cat = sATRIB;
                strcpy(token.lexeme, "<<");
                next_char();
            } else if (char_atual == '=') {
                token.cat = sMENORIGUAL;
                strcpy(token.lexeme, "<=");
                next_char();
            } else {
                token.cat = sMENOR;
                strcpy(token.lexeme, "<");
            }
            return token;
        }

        // Operadores compostos ou simples: >, >=
        if (char_atual == '>') {
            next_char();
            if (char_atual == '=') {
                token.cat = sMAIORIGUAL;
                strcpy(token.lexeme, ">=");
                next_char();
            } else {
                token.cat = sMAIOR;
                strcpy(token.lexeme, ">");
            }
            return token;
        }

        // Operador de implicacao: -> ou subtracao: -
        if (char_atual == '-') {
            next_char();
            if (char_atual == '>') {
                token.cat = sIMPLIC;
                strcpy(token.lexeme, "->");
                next_char();
            } else {
                token.cat = sSUBRAT;
                strcpy(token.lexeme, "-");
            }
            return token;
        }

        // Operadores logicos e relacionais com til: ~= (diferente) ou ~ (negacao)
        if (char_atual == '~') {
            next_char();
            if (char_atual == '=') {
                token.cat = sDIFERENTE;
                strcpy(token.lexeme, "~=");
                next_char();
            } else {
                token.cat = sNEG;
                strcpy(token.lexeme, "~");
            }
            return token;
        }

        // Delimitadores e operadores de um unico caractere
        char c = (char)char_atual;
        next_char();

        token.lexeme[0] = c;
        token.lexeme[1] = '\0';

        switch (c) {
            case '+': token.cat = sSOMA; return token;
            case '*': token.cat = sMULT; return token;
            case '=': token.cat = sIGUAL; return token;
            case '&': token.cat = sAND; return token;
            case '|': token.cat = sOR; return token;
            case '(': token.cat = sABREPAR; return token;
            case ')': token.cat = sFECHAPAR; return token;
            case '[': token.cat = sABRECOL; return token;
            case ']': token.cat = sFECHACOL; return token;
            case ';': token.cat = sPONTOVIRG; return token;
            case ',': token.cat = sVIRG; return token;
            case ':': token.cat = sDOISPONTOS; return token;
            default:
                diag_error_lex(token.line, "Caractere desconhecido ou invalido no alfabeto da linguagem.");
                break;
        }
    }

    token.cat = TOKEN_EOF;
    token.line = linha_atual;
    strcpy(token.lexeme, "EOF");
    return token;
}