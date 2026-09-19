#include "parser.h"
#include "lex.h"
#include "diag.h"
#include "symtab.h"

/*
    ASDR = Analisador Sintático Descendente Recursivo
    O papel do parser é consumir tokens do lexer e validar a gramática da linguagem.

    Aqui ficam apenas:
    - o estado atual do parser
    - avanço do token
    - match/consumo
    - funções dos não-terminais
    - chamadas recursivas

    O que NÃO fica aqui:
    - leitura do arquivo
    - opções da linha de comando
    - logs de token
    - geração de código


    Gramática:
    G = (, , P, <prg>)
    P:
    <prg> ::= [<gvars>] {<subs>} <princ>
    <gvars> ::= "sGLOBVARS" <decls>{<decls>}
    <lvars> ::= "sLOCVARS" <decls>{<decls>}
    <decls> ::= <id> {",“ <id>} "sKIND“ <type> ";“
    <id> ::= "sIDENTIF"
    <type> ::= (“sINT” | “sLOGIC" | “sCHR") [”[“ ”sCTEINT“ ”]“]
    <subs> ::= (<func> | <proc>)
    <func> ::= “sFUNC” <id> “(” [<param>] ”)” ”:” <type> [<lvars>] <bco>
    <proc> ::= “sPROC” <id> “(” [<param>] ”)” [<lvars>] <bco>
    <param> ::= [“sREF”] <id> ":“ <type> {",“ [“sREF”] <id> ":“ <type>}
    <bco> ::= “sSTART” {<cmd> ";“} ”sEND”
    <cmd> := <echo> | <get> | <case> | <chse> | <for> | <whle> |
    <rept> | <call> | <ret> | <atr>
    <vetr> ::= <id> ”[“ (”sCTEINT“ | <id>) ”]“
    <echo> ::= “sECHO” ”(“ <elem> {”,“ <elem>} ”)“
    <get> ::= “sGET” ”(“ (<id> | <vetr>) ”)“
    <case> ::= “sCASE” “(“ <expr> ) <cmd> [”sOTHERWISE” <cmd>] "sEND"
    <chse> ::= "sCHOOSE" "(" <expr> ")" <mlst> "sEND"
    <mlst> ::= <mtch> {<mtch>} [<other>]
    <mtch> ::= "sMATCH" <mvlrs> "sIMPLIC" <cmd> ";"
    <mvlrs> ::= "sCTEINT" {"," "sCTEINT"}
    <othr> ::= "sOTHERS" "sIMPLIC" <cmd> ";"
    <for> ::= “sFOR” <id> “sFROM“ (<id> | “sCTEINT“) “sTO“ (<id> |
    “sCTEINT“) [“sBY“ “sCTEINT“] “do“ <cmd>
    <whle> ::= “sWHILE” “(“ <expr> “)“ “sDO“ <cmd>
    Compiladores
    19
    <rept> ::= “sREPEAT” {<cmd> “;“} “sUNTIL” “(“ <expr> “)“
    <call> ::= <id> “(“ [expr {“,“ <expr>}] “)“
    <ret> ::= “sRETURN” <elem>
    <atr> ::= (<id> | <vetr>) “sATRIB” <elem>
    <elem> ::= <litl> | <id> | <vetr> | <call> | <expr>
    <litl> ::= “sSTRING” | “sCTEINT” | “sCTECHAR”
    <expr> ::= <elogc> {”sOR” <elogc>}
    <elogc> ::= <erlac> {”sAND” <erlac}
    <erlac> ::= <earit> {<oprel> <earit>}
    <earit> ::= <earip> {<opari> <earip>}
    <earip> ::= <fact> {<oparp> <fact>}
    <fact> ::= <elem> | “sNEG“ <fact> | “sSUBRAT“ <fact> | “(“<expr>“)“
    <oprel> ::= “sMAIOR” | “sMAIORIGUAL” | “sIGUAL” |
    “sMENOR” | “sMENORIGUAL” | “sDIFERENTE”
    <opari> ::= “sSOMA“ | “sSUBRAT“
    <oparp> ::= “sMULT“ | “sDIV“
*/

static Token current_tok;

static void advance(void) {
    current_tok = lex_next();
}

static void check(TokenCategoria expected) {
    if (current_tok.cat == expected) {
        advance();
    } else {
        diag_error_syntax(
            current_tok.line,
            token_cat_name(expected),
            token_cat_name(current_tok.cat)
        );
    }
}

/* Protótipos dos não-terminais para chamadas cruzadas/recursivas */
static void parse_expr(void);
static void parse_factor(void);
static void parse_cmd(void);
static void parse_block(void);

/* <elem> ::= <litl> | <id> | <vetr> | <call> | <expr> */
static void parse_elem(void) {
    if (current_tok.cat == sSTRING || current_tok.cat == sCTEINT || current_tok.cat == sCTECHAR) {
        advance();
    } else if (current_tok.cat == sIDENTIF) {
        advance();
        if (current_tok.cat == sABRECOL) { /* vetor */
            check(sABRECOL);
            if (current_tok.cat == sCTEINT || current_tok.cat == sIDENTIF) {
                advance();
            } else {
                diag_error_syntax(current_tok.line, "sCTEINT ou sIDENTIF", token_cat_name(current_tok.cat));
            }
            check(sFECHACOL);
        } else if (current_tok.cat == sABREPAR) { /* chamada de função */
            check(sABREPAR);
            if (current_tok.cat != sFECHAPAR) {
                parse_expr();
                while (current_tok.cat == sVIRG) {
                    check(sVIRG);
                    parse_expr();
                }
            }
            check(sFECHAPAR);
        }
    } else if (current_tok.cat == sABREPAR) {
        check(sABREPAR);
        parse_expr();
        check(sFECHAPAR);
    } else {
        diag_error_syntax(current_tok.line, "Elemento valido", token_cat_name(current_tok.cat));
    }
}

/* <fact> ::= <elem> | "sNEG" <fact> | "sSUBRAT" <fact> | "(" <expr> ")" */
static void parse_factor(void) {
    if (current_tok.cat == sNEG || current_tok.cat == sSUBRAT) {
        advance();
        parse_factor();
    } else {
        parse_elem();
    }
}

/* <earip> ::= <fact> {<oparp> <fact>} */
static void parse_earip(void) {
    parse_factor();
    while (current_tok.cat == sMULT || current_tok.cat == sDIV) {
        advance();
        parse_factor();
    }
}

/* <earit> ::= <earip> {<opari> <earip>} */
static void parse_earit(void) {
    parse_earip();
    while (current_tok.cat == sSOMA || current_tok.cat == sSUBRAT) {
        advance();
        parse_earip();
    }
}

/* <erlac> ::= <earit> {<oprel> <earit>} */
static void parse_erlac(void) {
    parse_earit();
    while (current_tok.cat == sMAIOR || current_tok.cat == sMAIORIGUAL || 
           current_tok.cat == sIGUAL || current_tok.cat == sMENOR || 
           current_tok.cat == sMENORIGUAL || current_tok.cat == sDIFERENTE) {
        advance();
        parse_earit();
    }
}

/* <elogc> ::= <erlac> {"sAND" <erlac>} */
static void parse_elogc(void) {
    parse_erlac();
    while (current_tok.cat == sAND) {
        check(sAND);
        parse_erlac();
    }
}

/* <expr> ::= <elogc> {"sOR" <elogc>} */
static void parse_expr(void) {
    parse_elogc();
    while (current_tok.cat == sOR) {
        check(sOR);
        parse_elogc();
    }
}

/* Exemplo de comando ECHO: "sECHO" "(" <elem> {"," <elem>} ")" */
static void parse_echo(void) {
    check(sECHO);
    check(sABREPAR);
    parse_elem();
    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        parse_elem();
    }
    check(sFECHAPAR);
}

static void parse_cmd(void) {
    switch (current_tok.cat) {
        case sECHO:
            parse_echo();
            break;
        /* Adicionar os demais comandos: sGET, sCASE, sCHOOSE, sFOR, sWHILE, sREPEAT, etc. */
        default:
            diag_error_syntax(current_tok.line, "Comando valido", token_cat_name(current_tok.cat));
            break;
    }
}

void parse_program(void) {
    advance(); /* Carrega o primeiro token */
    
    /* Implementar validações de <gvars>, <subs> e <princ> aqui */

    check(TOKEN_EOF); /* Garante término do arquivo */
}