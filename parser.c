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

    G = (, , P, <prg>)
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
    <echo> ::= “sECHO” ”(“ <elem> {”,“ <elem>} ”)“ ✅
    <get> ::= “sGET” ”(“ (<id> | <vetr>) ”)“ ✅
    <case> ::= “sCASE” “(“ <expr> ) <cmd> [”sOTHERWISE” <cmd>] "sEND" ✅
    <chse> ::= "sCHOOSE" "(" <expr> ")" <mlst> "sEND" ✅
    <mlst> ::= <mtch> {<mtch>} [<other>] ✅
    <mtch> ::= "sMATCH" <mvlrs> "sIMPLIC" <cmd> ";" ✅
    <mvlrs> ::= "sCTEINT" {"," "sCTEINT"} ✅
    <othr> ::= "sOTHERS" "sIMPLIC" <cmd> ";" ✅
    <for> ::= “sFOR” <id> “sFROM“ (<id> | “sCTEINT“) “sTO“ (<id> |
    “sCTEINT“) [“sBY“ “sCTEINT“] “do“ <cmd> ✅
    <whle> ::= “sWHILE” “(“ <expr> “)“ “sDO“ <cmd> ✅
    Compiladores
    19
    <rept> ::= “sREPEAT” {<cmd> “;“} “sUNTIL” “(“ <expr> “)“ ✅
    <call> ::= <id> “(“ [expr {“,“ <expr>}] “)“ ✅
    <ret> ::= “sRETURN” <elem> ✅
    <atr> ::= (<id> | <vetr>) “sATRIB” <elem> ✅
    <elem> ::= <litl> | <id> | <vetr> | <call> | <expr> ✅
    <litl> ::= “sSTRING” | “sCTEINT” | “sCTECHAR”
    <expr> ::= <elogc> {”sOR” <elogc>} ✅
    <elogc> ::= <erlac> {”sAND” <erlac} ✅
    <erlac> ::= <earit> {<oprel> <earit>} ✅
    <earit> ::= <earip> {<opari> <earip>} ✅
    <earip> ::= <fact> {<oparp> <fact>} ✅
    <fact> ::= <elem> | “sNEG“ <fact> | “sSUBRAT“ <fact> | “(“<expr>“)“ ✅
    <oprel> ::= “sMAIOR” | “sMAIORIGUAL” | “sIGUAL” | “sMENOR” | “sMENORIGUAL” | “sDIFERENTE” ✅
    <opari> ::= “sSOMA“ | “sSUBRAT“ ✅
    <oparp> ::= “sMULT“ | “sDIV“ ✅
    proc/func/param/subs ✅

    ATENÇÃO — nomes de token assumidos (confira contra o seu lex.h):
    sDOISPONTOS (":"), sSTART, sEND, sDO, sFROM, sTO, sBY, sOTHERWISE,
    sOTHERS, sIMPLIC ("->"), sMATCH, sCHOOSE, sFOR, sWHILE, sREPEAT,
    sUNTIL, sRETURN, sATRIB, sREF, sCTECHAR.

    NOTA sobre <case>/<for>/<whle>: a EBNF do apêndice usa apenas um único
    <cmd> no corpo e não menciona "end" nessas três produções. Isso diverge
    do texto do manual e dos exemplos oficiais (Programas 4, 5 e 6), que
    mostram múltiplos comandos terminados por "end". Optamos por seguir o
    comportamento demonstrado nos exemplos, já que são a referência usada
    para os testes de avaliação.

    NOTA sobre <princ>: o apêndice não define <princ> separadamente de
    <subs>. Sintaticamente, main() tem exatamente a mesma forma de um
    <proc> qualquer, então tratamos {<subs>} como um laço que já inclui a
    main (o último proc do arquivo). A verificação de que existe uma main
    válida (nome, sem retorno, etc.) é uma questão semântica, tratada na
    fase 2 junto à tabela de símbolos.
*/

static Token current_tok;

/* Buffer de 1 token de lookahead além do current_tok. Necessário porque
   <call> e <atr> (e <id>/<vetr> dentro de <elem>) começam ambos com
   sIDENTIF: precisamos olhar o token seguinte para decidir qual função
   do não-terminal chamar, sem "roubar" a responsabilidade de consumir
   o próprio <id> de cada uma dessas funções. */
static Token lookahead_tok;
static int has_lookahead = 0;

static void advance(void) {
    if (has_lookahead) {
        current_tok = lookahead_tok;
        has_lookahead = 0;
    } else {
        current_tok = lex_next();
    }
}

static Token peek(void) {
    if (!has_lookahead) {
        lookahead_tok = lex_next();
        has_lookahead = 1;
    }
    return lookahead_tok;
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
static void parse_expr(void);   // ✅
static void parse_factor(void); // ✅
static void parse_cmd(void);    // ✅
static void parse_block(void);  // ✅ <bco>

/* Protótipos de declarações e tipos */
static void parse_gvars(void);  // ✅
static void parse_lvars(void);  // ✅
static void parse_decls(void);  // ✅
static void parse_type(void);   // ✅
static void parse_id(void);     // ✅

/* Protótipos sub-rotinas e Parâmetros */
static void parse_subs(void);   // ✅
static void parse_func(void);   // ✅
static void parse_proc(void);   // ✅
static void parse_param(void);  // ✅

/* Protótipo de comandos restantes */
static void parse_echo(void);   // ✅
static void parse_get(void);    // ✅
static void parse_case(void);   // ✅
static void parse_chse(void);   // ✅
static void parse_mlst(void);   // ✅
static void parse_mtch(void);   // ✅
static void parse_mvlrs(void);  // ✅
static void parse_othr(void);   // ✅
static void parse_vetr(void);   // ✅
static void parse_for(void);    // ✅
static void parse_whle(void);   // ✅
static void parse_rept(void);   // ✅
static void parse_call(void);   // ✅
static void parse_ret(void);    // ✅
static void parse_atr(void);    // ✅
static void parse_elem(void);   // ✅
static void parse_cmd_list(void); // auxiliar (não é não-terminal da gramática)


/* <decls> ::= <id> {",“ <id>} "sKIND“ <type> ";“ */
static void parse_decls(void) {
    parse_id();
    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        parse_id();
    }
    check(sKIND);
    parse_type();
    check(sPONTOVIRG);
}

/* <gvars> ::= "sGLOBVARS" <decls>{<decls>} */
static void parse_gvars(void) {
    check(sGLOBVARS);
    parse_decls();
    while (current_tok.cat == sIDENTIF) {
        parse_decls();
    }
}

/* <lvars> ::= "sLOCVARS" <decls>{<decls>} */
static void parse_lvars(void) {
    check(sLOCVARS);
    parse_decls();
    while (current_tok.cat == sIDENTIF) {
        parse_decls();
    }
}

/* <type> ::= (“sINT” | “sLOGIC" | “sCHR") [”[“ ”sCTEINT“ ”]“] */
static void parse_type(void) {
    if (current_tok.cat == sINT || current_tok.cat == sLOGIC || current_tok.cat == sCHR) {
        advance();
    } else {
        diag_error_syntax(current_tok.line, "sINT, sLOGIC ou sCHR", token_cat_name(current_tok.cat));
    }

    if (current_tok.cat == sABRECOL) {
        check(sABRECOL);
        check(sCTEINT);
        check(sFECHACOL);
    }
}

/* <id> ::= "sIDENTIF" */
static void parse_id(void) {
    check(sIDENTIF);
}

/* <param> ::= ["sREF"] <id> ":" <type> {"," ["sREF"] <id> ":" <type>} */
static void parse_param(void) {
    if (current_tok.cat == sREF) {
        check(sREF);
    }
    parse_id();
    check(sDOISPONTOS);
    parse_type();

    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        if (current_tok.cat == sREF) {
            check(sREF);
        }
        parse_id();
        check(sDOISPONTOS);
        parse_type();
    }
}

/* <func> ::= "sFUNC" <id> "(" [<param>] ")" ":" <type> [<lvars>] <bco> */
static void parse_func(void) {
    check(sFUNC);
    parse_id();
    check(sABREPAR);
    if (current_tok.cat != sFECHAPAR) {
        parse_param();
    }
    check(sFECHAPAR);
    check(sDOISPONTOS);
    parse_type();
    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_block();
}

/* <proc> ::= "sPROC" <id> "(" [<param>] ")" [<lvars>] <bco> */
static void parse_proc(void) {
    check(sPROC);
    parse_id();
    check(sABREPAR);
    if (current_tok.cat != sFECHAPAR) {
        parse_param();
    }
    check(sFECHAPAR);
    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_block();
}

/* <subs> ::= (<func> | <proc>) */
static void parse_subs(void) {
    if (current_tok.cat == sFUNC) {
        parse_func();
    } else if (current_tok.cat == sPROC) {
        parse_proc();
    } else {
        diag_error_syntax(current_tok.line, "sFUNC ou sPROC", token_cat_name(current_tok.cat));
    }
}

/* <bco> ::= "sSTART" {<cmd> ";"} "sEND" */
static void parse_block(void) {
    check(sSTART);
    parse_cmd_list();
    check(sEND);
}

/* Auxiliar: trata a repetição {<cmd> ";"} usada em <bco>, <case>, <for>,
   <whle> e <rept>. Não corresponde a um não-terminal próprio da gramática,
   apenas evita duplicar o mesmo laço em cinco lugares diferentes. */
static void parse_cmd_list(void) {
    while (current_tok.cat == sECHO   || current_tok.cat == sGET    ||
           current_tok.cat == sCASE   || current_tok.cat == sCHOOSE ||
           current_tok.cat == sFOR    || current_tok.cat == sWHILE  ||
           current_tok.cat == sREPEAT || current_tok.cat == sRETURN ||
           current_tok.cat == sIDENTIF) {
        parse_cmd();
        check(sPONTOVIRG);
    }
}

/* <vetr> ::= <id> "[" ("sCTEINT" | <id>) "]" */
static void parse_vetr(void) {
    parse_id();
    check(sABRECOL);
    if (current_tok.cat == sCTEINT || current_tok.cat == sIDENTIF) {
        advance();
    } else {
        diag_error_syntax(current_tok.line, "sCTEINT ou sIDENTIF", token_cat_name(current_tok.cat));
    }
    check(sFECHACOL);
}

/* <call> ::= <id> "(" [<expr> {"," <expr>}] ")" */
static void parse_call(void) {
    parse_id();
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

/* <atr> ::= (<id> | <vetr>) "sATRIB" <elem>
   <id> e <vetr> compartilham o mesmo prefixo (sIDENTIF); usamos peek()
   para decidir sem consumir o token antes da hora. */
static void parse_atr(void) {
    if (peek().cat == sABRECOL) {
        parse_vetr();
    } else {
        parse_id();
    }
    check(sATRIB);
    parse_elem();
}

/* <ret> ::= "sRETURN" <elem> */
static void parse_ret(void) {
    check(sRETURN);
    parse_elem();
}

/* <echo> ::= "sECHO" "(" <elem> {"," <elem>} ")" */
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

/* <get> ::= "sGET" "(" (<id> | <vetr>) ")" */
static void parse_get(void) {
    check(sGET);
    check(sABREPAR);
    if (peek().cat == sABRECOL) {
        parse_vetr();
    } else {
        parse_id();
    }
    check(sFECHAPAR);
}

/* <case> ::= "sCASE" "(" <expr> ")" {<cmd> ";"} ["sOTHERWISE" {<cmd> ";"}] "sEND"
   (ver nota no cabeçalho do arquivo sobre a divergência com o apêndice) */
static void parse_case(void) {
    check(sCASE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
    parse_cmd_list();
    if (current_tok.cat == sOTHERWISE) {
        check(sOTHERWISE);
        parse_cmd_list();
    }
    check(sEND);
}

/* <mvlrs> ::= "sCTEINT" {"," "sCTEINT"} */
static void parse_mvlrs(void) {
    check(sCTEINT);
    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        check(sCTEINT);
    }
}

/* <mtch> ::= "sMATCH" <mvlrs> "sIMPLIC" <cmd> ";" */
static void parse_mtch(void) {
    check(sMATCH);
    parse_mvlrs();
    check(sIMPLIC);
    parse_cmd();
    check(sPONTOVIRG);
}

/* <othr> ::= "sOTHERS" "sIMPLIC" <cmd> ";" */
static void parse_othr(void) {
    check(sOTHERS);
    check(sIMPLIC);
    parse_cmd();
    check(sPONTOVIRG);
}

/* <mlst> ::= <mtch> {<mtch>} [<othr>] */
static void parse_mlst(void) {
    parse_mtch();
    while (current_tok.cat == sMATCH) {
        parse_mtch();
    }
    if (current_tok.cat == sOTHERS) {
        parse_othr();
    }
}

/* <chse> ::= "sCHOOSE" "(" <expr> ")" <mlst> "sEND" */
static void parse_chse(void) {
    check(sCHOOSE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
    parse_mlst();
    check(sEND);
}

/* <for> ::= "sFOR" <id> "sFROM" (<id>|"sCTEINT") "sTO" (<id>|"sCTEINT")
             ["sBY" "sCTEINT"] "sDO" {<cmd> ";"} "sEND"
   (ver nota no cabeçalho sobre "end" e lista de comandos) */
static void parse_for(void) {
    check(sFOR);
    parse_id();
    check(sFROM);
    if (current_tok.cat == sIDENTIF || current_tok.cat == sCTEINT) {
        advance();
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF ou sCTEINT", token_cat_name(current_tok.cat));
    }
    check(sTO);
    if (current_tok.cat == sIDENTIF || current_tok.cat == sCTEINT) {
        advance();
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF ou sCTEINT", token_cat_name(current_tok.cat));
    }
    if (current_tok.cat == sBY) {
        check(sBY);
        check(sCTEINT);
    }
    check(sDO);
    parse_cmd_list();
    check(sEND);
}

/* <whle> ::= "sWHILE" "(" <expr> ")" "sDO" {<cmd> ";"} "sEND"
   (ver nota no cabeçalho sobre "end" e lista de comandos) */
static void parse_whle(void) {
    check(sWHILE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
    check(sDO);
    parse_cmd_list();
    check(sEND);
}

/* <rept> ::= "sREPEAT" {<cmd> ";"} "sUNTIL" "(" <expr> ")" */
static void parse_rept(void) {
    check(sREPEAT);
    parse_cmd_list();
    check(sUNTIL);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
}

/* <cmd> ::= <echo> | <get> | <case> | <chse> | <for> | <whle> |
             <rept> | <call> | <ret> | <atr>

   <call> e <atr> começam ambos com sIDENTIF: usamos peek() apenas para
   decidir qual função chamar, sem consumir o id aqui — cada função
   consome o seu próprio <id>. */
static void parse_cmd(void) {
    switch (current_tok.cat) {
        case sECHO:
            parse_echo();
            break;
        case sGET:
            parse_get();
            break;
        case sCASE:
            parse_case();
            break;
        case sCHOOSE:
            parse_chse();
            break;
        case sFOR:
            parse_for();
            break;
        case sWHILE:
            parse_whle();
            break;
        case sREPEAT:
            parse_rept();
            break;
        case sRETURN:
            parse_ret();
            break;
        case sIDENTIF:
            if (peek().cat == sABREPAR) {
                parse_call();
            } else {
                parse_atr();
            }
            break;
        default:
            diag_error_syntax(current_tok.line, "comando valido", token_cat_name(current_tok.cat));
            break;
    }
}

/* <elem> ::= <litl> | <id> | <vetr> | <call>
   Observação: a alternativa <expr> listada para <elem> no apêndice é
   redundante/inacessível aqui — expressões entre parênteses já são
   tratadas em <fact> como "(" <expr> ")"; se <elem> também aceitasse
   <expr> diretamente teríamos recursão sem consumo de token. */
static void parse_elem(void) {
    /* <litl> ::= "sSTRING" | "sCTEINT" | "sCTECHAR" */
    if (current_tok.cat == sSTRING || current_tok.cat == sCTEINT || current_tok.cat == sCTECHAR) {
        advance();
        return;
    }

    if (current_tok.cat == sIDENTIF) {
        TokenCategoria prox = peek().cat;
        if (prox == sABRECOL) {
            parse_vetr();
        } else if (prox == sABREPAR) {
            parse_call();
        } else {
            parse_id();
        }
        return;
    }

    diag_error_syntax(current_tok.line, "elemento valido (literal, identificador, vetor ou chamada)", token_cat_name(current_tok.cat));
}

/* <fact> ::= <elem> | "sNEG" <fact> | "sSUBRAT" <fact> | "(" <expr> ")" */
static void parse_factor(void) {
    if (current_tok.cat == sNEG || current_tok.cat == sSUBRAT) {
        advance();
        parse_factor();
    } else if (current_tok.cat == sABREPAR) {
        check(sABREPAR);
        parse_expr();
        check(sFECHAPAR);
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

/* <prg> ::= [<gvars>] {<subs>} <princ>
   (ver nota no cabeçalho sobre <princ> não ser tratado separadamente aqui) */
void parse_program(void) {
    advance();

    if (current_tok.cat == sGLOBVARS) {
        parse_gvars();
    }

    while (current_tok.cat == sFUNC || current_tok.cat == sPROC) {
        parse_subs();
    }

    check(TOKEN_EOF); /* Garante término do arquivo */
}