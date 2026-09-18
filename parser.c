#include "parser.h"
#include "lex.h"
#include "symtab.h"
#include "diag.h"
#include "log.h"
#include "opt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token current_tok;
static int block_counter = 0;

/* Obtem o proximo token do analisador lexico e, se ativo, grava no arquivo de log .tk */
static void advance(void) {
    current_tok = lex_next();
    if (opts_get()->gen_tokens) {
        log_token(&current_tok);
    }
}

/* Metodo check/match classico de ASDR: valida o token esperado e consome o proximo */
static void check(TokenCategoria expected) {
    if (current_tok.cat == expected) {
        advance();
    } else {
        diag_error_syntax(current_tok.line, token_cat_name(expected), token_cat_name(current_tok.cat));
    }
}

/* Declaracoes adiantadas das funcoes do ASDR conforme os nao-terminais da EBNF */
static void parse_gvars(void);
static void parse_lvars(void);
static void parse_decls(SymCategory cat);
static void parse_type(char *out_type, int *out_size);
static void parse_subs(void);
static void parse_func(void);
static void parse_proc(void);
static void parse_param(int *param_count);
static void parse_bco(void);
static void parse_cmd(void);
static void parse_echo(void);
static void parse_get(void);
static void parse_case(void);
static void parse_chse(void);
static void parse_for(void);
static void parse_whle(void);
static void parse_rept(void);
static void parse_call(void);
static void parse_ret(void);
static void parse_atr_or_call(void);
static void parse_elem(void);
static void parse_expr(void);
static void parse_elogc(void);
static void parse_erlac(void);
static void parse_earit(void);
static void parse_earip(void);
static void parse_fact(void);

/* <type> ::= ("sINT" | "sLOGIC" | "sCHR") ["[" "sCTEINT" "]"] */
static void parse_type(char *out_type, int *out_size) {
    *out_size = 0;
    if (current_tok.cat == sINT) {
        strcpy(out_type, "int");
        check(sINT);
    } else if (current_tok.cat == sLOGIC) {
        strcpy(out_type, "logic");
        check(sLOGIC);
    } else if (current_tok.cat == sCHR) {
        strcpy(out_type, "chr");
        check(sCHR);
    } else {
        diag_error_syntax(current_tok.line, "sINT, sLOGIC ou sCHR", token_cat_name(current_tok.cat));
    }

    if (current_tok.cat == sABRECOL) {
        check(sABRECOL);
        if (current_tok.cat == sCTEINT) {
            *out_size = atoi(current_tok.lexeme);
            check(sCTEINT);
        } else {
            diag_error_syntax(current_tok.line, "sCTEINT", token_cat_name(current_tok.cat));
        }
        check(sFECHACOL);
    }
}

/* <decls> ::= <id> {"," <id>} "sKIND" <type> ";" */
static void parse_decls(SymCategory cat) {
    char id_list[64][64];
    int count = 0;

    if (current_tok.cat == sIDENTIF) {
        strncpy(id_list[count++], current_tok.lexeme, 63);
        check(sIDENTIF);
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF", token_cat_name(current_tok.cat));
    }

    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        if (current_tok.cat == sIDENTIF) {
            if (count < 64) {
                strncpy(id_list[count++], current_tok.lexeme, 63);
            }
            check(sIDENTIF);
        } else {
            diag_error_syntax(current_tok.line, "sIDENTIF", token_cat_name(current_tok.cat));
        }
    }

    check(sKIND); // Palavra reservada 'is'

    char type_name[64];
    int size = 0;
    parse_type(type_name, &size);
    check(sPONTOVIRG);

    // Registo de todas as variaveis na Tabela de Simbolos
    for (int i = 0; i < count; i++) {
        if (!symtab_insert(id_list[i], cat, type_name, size)) {
            diag_error(current_tok.line, "Identificador redeclarado no mesmo escopo.");
        }
    }
}

/* <gvars> ::= "sGLOBVARS" <decls> {<decls>} */
static void parse_gvars(void) {
    check(sGLOBVARS);
    parse_decls(SYM_VAR_GLOBAL);
    while (current_tok.cat == sIDENTIF) {
        parse_decls(SYM_VAR_GLOBAL);
    }
}

/* <lvars> ::= "sLOCVARS" <decls> {<decls>} */
static void parse_lvars(void) {
    check(sLOCVARS);
    parse_decls(SYM_VAR_LOCAL);
    while (current_tok.cat == sIDENTIF) {
        parse_decls(SYM_VAR_LOCAL);
    }
}

/* <param> ::= ["sREF"] <id> ":" <type> {"," ["sREF"] <id> ":" <type>} */
static void parse_param(int *param_count) {
    *param_count = 0;
    while (1) {
        if (current_tok.cat == sREF) {
            check(sREF);
        }

        char pid[64];
        if (current_tok.cat == sIDENTIF) {
            strncpy(pid, current_tok.lexeme, 63);
            check(sIDENTIF);
        } else {
            diag_error_syntax(current_tok.line, "sIDENTIF", token_cat_name(current_tok.cat));
        }

        check(sDOISPONTOS);
        char ptype[64];
        int psize = 0;
        parse_type(ptype, &psize);

        if (!symtab_insert(pid, SYM_PARAM, ptype, psize)) {
            diag_error(current_tok.line, "Parametro repetido na assinatura da sub-rotina.");
        }
        (*param_count)++;

        if (current_tok.cat == sVIRG) {
            check(sVIRG);
        } else {
            break;
        }
    }
}

/* <bco> ::= "sSTART" {<cmd> ";"} "SEND" */
static void parse_bco(void) {
    check(sSTART);
    while (current_tok.cat != sEND && current_tok.cat != TOKEN_EOF) {
        parse_cmd();
        check(sPONTOVIRG);
    }
    check(sEND);
}

/* <func> ::= "sFUNC" <id> "(" [<param>] ")" ":" <type> [<lvars>] <bco> */
static void parse_func(void) {
    check(sFUNC);

    char fid[64];
    if (current_tok.cat == sIDENTIF) {
        strncpy(fid, current_tok.lexeme, 63);
        check(sIDENTIF);
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF", token_cat_name(current_tok.cat));
    }

    // Abre escopo de funcao para registro de parametros e variaveis locais
    char scope_name[128];
    snprintf(scope_name, sizeof(scope_name), "fn:%s.locals", fid);
    symtab_enter_scope(scope_name);

    check(sABREPAR);
    int params = 0;
    if (current_tok.cat == sREF || current_tok.cat == sIDENTIF) {
        parse_param(&params);
    }
    check(sFECHAPAR);
    check(sDOISPONTOS);

    char ret_type[64];
    int rsize = 0;
    parse_type(ret_type, &rsize);

    // Registra a sub-rotina no escopo global
    symtab_leave_scope();
    if (!symtab_insert(fid, SYM_FUNC, ret_type, params)) {
        diag_error(current_tok.line, "Funcao com identificador duplicado.");
    }
    symtab_enter_scope(scope_name);

    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_bco();
    symtab_leave_scope();
}

/* <proc> ::= "sPROC" <id> "(" [<param>] ")" [<lvars>] <bco> */
static void parse_proc(void) {
    check(sPROC);

    char pid[64];
    if (current_tok.cat == sIDENTIF) {
        strncpy(pid, current_tok.lexeme, 63);
        check(sIDENTIF);
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF", token_cat_name(current_tok.cat));
    }

    char scope_name[128];
    snprintf(scope_name, sizeof(scope_name), "proc:%s.locals", pid);
    symtab_enter_scope(scope_name);

    check(sABREPAR);
    int params = 0;
    if (current_tok.cat == sREF || current_tok.cat == sIDENTIF) {
        parse_param(&params);
    }
    check(sFECHAPAR);

    symtab_leave_scope();
    if (!symtab_insert(pid, SYM_PROC, "void", params)) {
        diag_error(current_tok.line, "Procedimento com identificador duplicado.");
    }
    symtab_enter_scope(scope_name);

    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_bco();
    symtab_leave_scope();
}

/* <subs> ::= { <func> | <proc> } */
static void parse_subs(void) {
    while (current_tok.cat == sFUNC || current_tok.cat == sPROC) {
        if (current_tok.cat == sFUNC) {
            parse_func();
        } else {
            parse_proc();
        }
    }
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
    check(sIDENTIF);
    if (current_tok.cat == sABRECOL) {
        check(sABRECOL);
        if (current_tok.cat == sCTEINT) {
            check(sCTEINT);
        } else {
            check(sIDENTIF);
        }
        check(sFECHACOL);
    }
    check(sFECHAPAR);
}

/* <case> ::= "sCASE" "(" <expr> ")" <cmd> ["sOTHERWISE" <cmd>] "sEND" */
static void parse_case(void) {
    check(sCASE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);

    parse_cmd();

    if (current_tok.cat == sOTHERWISE) {
        check(sOTHERWISE);
        parse_cmd();
    }
    check(sEND);
}

/* <chse> ::= "sCHOOSE" "(" <expr> ")" <mlst> "sEND" */
static void parse_chse(void) {
    check(sCHOOSE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);

    while (current_tok.cat == sMATCH) {
        check(sMATCH);
        check(sCTEINT);
        while (current_tok.cat == sVIRG) {
            check(sVIRG);
            check(sCTEINT);
        }
        check(sIMPLIC);
        parse_cmd();
        check(sPONTOVIRG);
    }

    if (current_tok.cat == sOTHERS) {
        check(sOTHERS);
        check(sIMPLIC);
        parse_cmd();
        check(sPONTOVIRG);
    }

    check(sEND);
}

/* <for> ::= "sFOR" <id> "sFROM" (<id> | "sCTEINT") "sTO" (<id> | "sCTEINT") ["sBY" "sCTEINT"] "sDO" <cmd> */
static void parse_for(void) {
    check(sFOR);
    check(sIDENTIF);
    check(sFROM);
    if (current_tok.cat == sIDENTIF) {
        check(sIDENTIF);
    } else {
        check(sCTEINT);
    }

    check(sTO);
    if (current_tok.cat == sIDENTIF) {
        check(sIDENTIF);
    } else {
        check(sCTEINT);
    }

    if (current_tok.cat == sBY) {
        check(sBY);
        if (current_tok.cat == sSUBRAT) {
            check(sSUBRAT);
        }
        check(sCTEINT);
    }

    check(sDO);
    parse_cmd();
    if (current_tok.cat == sEND) {
        check(sEND);
    }
}

/* <whle> ::= "sWHILE" "(" <expr> ")" "sDO" <cmd> */
static void parse_whle(void) {
    check(sWHILE);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
    check(sDO);
    parse_cmd();
    if (current_tok.cat == sEND) {
        check(sEND);
    }
}

/* <rept> ::= "sREPEAT" {<cmd> ";"} "sUNTIL" "(" <expr> ")" */
static void parse_rept(void) {
    check(sREPEAT);
    while (current_tok.cat != sUNTIL && current_tok.cat != TOKEN_EOF) {
        parse_cmd();
        check(sPONTOVIRG);
    }
    check(sUNTIL);
    check(sABREPAR);
    parse_expr();
    check(sFECHAPAR);
}

/* <ret> ::= "sRETURN" <elem> */
static void parse_ret(void) {
    check(sRETURN);
    parse_elem();
}

/* Chamada de procedimento/funcao ou Atribuicao: <atr> ::= (<id> | <vetr>) "sATRIB" <elem> */
static void parse_atr_or_call(void) {
    check(sIDENTIF);

    if (current_tok.cat == sABRECOL) {
        // Elemento vetorizado: id[indice] << elem
        check(sABRECOL);
        if (current_tok.cat == sCTEINT) {
            check(sCTEINT);
        } else {
            check(sIDENTIF);
        }
        check(sFECHACOL);
        check(sATRIB);
        parse_elem();
    } else if (current_tok.cat == sATRIB) {
        check(sATRIB);
        parse_elem();
    } else if (current_tok.cat == sABREPAR) {
        // Chamada de procedimento: id(args)
        check(sABREPAR);
        if (current_tok.cat != sFECHAPAR) {
            parse_expr();
            while (current_tok.cat == sVIRG) {
                check(sVIRG);
                parse_expr();
            }
        }
        check(sFECHAPAR);
    } else {
        diag_error_syntax(current_tok.line, "'<<' ou '('", token_cat_name(current_tok.cat));
    }
}

/* <cmd> ::= <echo> | <get> | <case> | <chse> | <for> | <whle> | <rept> | <ret> | <atr_or_call> | <bco> */
static void parse_cmd(void) {
    switch (current_tok.cat) {
        case sECHO:      parse_echo(); break;
        case sGET:       parse_get(); break;
        case sCASE:      parse_case(); break;
        case sCHOOSE:    parse_chse(); break;
        case sFOR:       parse_for(); break;
        case sWHILE:     parse_whle(); break;
        case sREPEAT:    parse_rept(); break;
        case sRETURN:    parse_ret(); break;
        case sIDENTIF:   parse_atr_or_call(); break;
        case sSTART:     parse_bco(); break;
        default:
            diag_error_syntax(current_tok.line, "comando valido", token_cat_name(current_tok.cat));
            break;
    }
}

/* <fact> ::= <elem> | "sNEG" <fact> | "sSUBRAT" <fact> | "(" <expr> ")" */
static void parse_fact(void) {
    if (current_tok.cat == sNEG) {
        check(sNEG);
        parse_fact();
    } else if (current_tok.cat == sSUBRAT) {
        check(sSUBRAT);
        parse_fact();
    } else if (current_tok.cat == sABREPAR) {
        check(sABREPAR);
        parse_expr();
        check(sFECHAPAR);
    } else {
        parse_elem();
    }
}

/* <earip> ::= <fact> { ("sMULT" | "sDIV") <fact> } */
static void parse_earip(void) {
    parse_fact();
    while (current_tok.cat == sMULT || current_tok.cat == sDIV) {
        if (current_tok.cat == sMULT) check(sMULT);
        else check(sDIV);
        parse_fact();
    }
}

/* <earit> ::= <earip> { ("sSOMA" | "sSUBRAT") <earip> } */
static void parse_earit(void) {
    parse_earip();
    while (current_tok.cat == sSOMA || current_tok.cat == sSUBRAT) {
        if (current_tok.cat == sSOMA) check(sSOMA);
        else check(sSUBRAT);
        parse_earip();
    }
}

/* <erlac> ::= <earit> { <oprel> <earit> } */
static void parse_erlac(void) {
    parse_earit();
    while (current_tok.cat == sMAIOR || current_tok.cat == sMAIORIGUAL ||
           current_tok.cat == sMENOR || current_tok.cat == sMENORIGUAL ||
           current_tok.cat == sIGUAL || current_tok.cat == sDIFERENTE) {
        advance();
        parse_earit();
    }
}

/* <elogc> ::= <erlac> { "sAND" <erlac> } */
static void parse_elogc(void) {
    parse_erlac();
    while (current_tok.cat == sAND) {
        check(sAND);
        parse_erlac();
    }
}

/* <expr> ::= <elogc> { "sOR" <elogc> } */
static void parse_expr(void) {
    parse_elogc();
    while (current_tok.cat == sOR) {
        check(sOR);
        parse_elogc();
    }
}

/* <elem> ::= <litl> | <id> | <vetr> | <call> | <expr> */
static void parse_elem(void) {
    if (current_tok.cat == sSTRING) {
        check(sSTRING);
    } else if (current_tok.cat == sCTEINT) {
        check(sCTEINT);
    } else if (current_tok.cat == sCTECHAR) {
        check(sCTECHAR);
    } else if (current_tok.cat == sIDENTIF) {
        check(sIDENTIF);
        if (current_tok.cat == sABRECOL) {
            check(sABRECOL);
            if (current_tok.cat == sCTEINT) check(sCTEINT);
            else check(sIDENTIF);
            check(sFECHACOL);
        } else if (current_tok.cat == sABREPAR) {
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
        diag_error_syntax(current_tok.line, "elemento literal, variavel ou expressao", token_cat_name(current_tok.cat));
    }
}

/* <prg> ::= [<gvars>] {<subs>} <proc_main> */
void parse_program(void) {
    advance(); // Carrega o primeiro token

    if (current_tok.cat == sGLOBVARS) {
        parse_gvars();
    }

    parse_subs();

    if (current_tok.cat == TOKEN_EOF) {
        diag_info("Analise sintatica concluida com sucesso.");
    } else {
        diag_error_syntax(current_tok.line, "EOF", token_cat_name(current_tok.cat));
    }
}