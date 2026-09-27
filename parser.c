#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

*/

static Token current_tok;

/* Buffer de 1 token de lookahead além do current_tok. Necessário porque
   <call> e <atr> (e <id>/<vetr> dentro de <elem>) começam ambos com
   sIDENTIF: precisamos olhar o token seguinte para decidir qual função
   do não-terminal chamar, sem "roubar" a responsabilidade de consumir
   o próprio <id> de cada uma dessas funções. */
static Token lookahead_tok;
static int has_lookahead = 0;
static int saw_main = 0;

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
            tokenCatNome(expected),
            tokenCatNome(current_tok.cat)
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
static void parse_decls(SimboloCat cat);  // ✅ (cat: variáveis globais ou locais)
static SimboloTipo parse_type(int *extra_out); // ✅ (extra_out: tamanho do vetor, ou NULL se não interessa)
static void parse_id(void);     // ✅

/* Protótipos sub-rotinas e Parâmetros */
static void parse_subs(void);   // ✅
static void parse_func(void);   // ✅
static void parse_proc(void);   // ✅
static int parse_param(void);   // ✅ retorna a quantidade de parâmetros lidos

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
static void parse_atom(void);
static void parse_cmd_list(void); // auxiliar (não é não-terminal da gramática)

static void require_declared(const char *name, int line) {
    if (symtab_lookup(name) == NULL) {
        char msg[160];
        snprintf(msg, sizeof(msg), "identificador '%s' nao declarado", name);
        diag_error(line, msg);
    }
}

static void parse_use_id(void) {
    char name[32];
    int line = current_tok.line;
    strncpy(name, current_tok.lexeme, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    parse_id();
    require_declared(name, line);
}


/* <decls> ::= <id> {",“ <id>} "sKIND“ <type> ";“
   cat indica se estes identificadores são variáveis globais ou locais,
   já que a mesma produção <decls> é reaproveitada por <gvars> e <lvars>,
   mas cada uso deve gerar símbolos de categoria diferente na TS. */
#define PARSE_DECLS_MAX_IDS 64
static void parse_decls(SimboloCat cat) {
    char nomes[PARSE_DECLS_MAX_IDS][32];
    int n = 0;
    int i;
    SimboloTipo tipo;
    int extra = 0;

    /* Guardamos os lexemas porque current_tok muda a cada advance();
       só sabemos o tipo/tamanho no final da produção (depois de "sKIND"),
       então a inserção na TS fica para depois de parse_type(). */
    strncpy(nomes[n], current_tok.lexeme, sizeof(nomes[n]) - 1);
    nomes[n][sizeof(nomes[n]) - 1] = '\0';
    n++;
    parse_id();

    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        if (n < PARSE_DECLS_MAX_IDS) {
            strncpy(nomes[n], current_tok.lexeme, sizeof(nomes[n]) - 1);
            nomes[n][sizeof(nomes[n]) - 1] = '\0';
            n++;
        }
        parse_id();
    }

    check(sKIND);
    tipo = parse_type(&extra);
    check(sPONTOVIRG);

    for (i = 0; i < n; i++) {
        if (!symtab_insert(nomes[i], cat, tipo, extra)) {
            diag_error_syntax(current_tok.line, "identificador nao duplicado no escopo", nomes[i]);
        }
    }
}

/* <gvars> ::= "sGLOBVARS" <decls>{<decls>} */
static void parse_gvars(void) {
    check(sGLOBVARS);
    parse_decls(CAT_VAR_GLOBAL);
    while (current_tok.cat == sIDENTIF) {
        parse_decls(CAT_VAR_GLOBAL);
    }
}

/* <lvars> ::= "sLOCVARS" <decls>{<decls>} */
static void parse_lvars(void) {
    check(sLOCVARS);
    parse_decls(CAT_VAR_LOCAL);
    while (current_tok.cat == sIDENTIF) {
        parse_decls(CAT_VAR_LOCAL);
    }
}

/* <type> ::= (“sINT” | “sLOGIC" | “sCHR") [”[“ ”sCTEINT“ ”]“]
   extra_out recebe o tamanho do vetor quando presente, ou 0 quando a
   declaração não é um vetor. Pode ser NULL quando quem chama não precisa
   dessa informação (ex.: tipo de retorno de função, que nunca é vetor). */
static SimboloTipo parse_type(int *extra_out) {
    SimboloTipo tipo;

    if (current_tok.cat == sINT) {
        tipo = TIPO_INT;
        advance();
    } else if (current_tok.cat == sLOGIC) {
        tipo = TIPO_BOOL;
        advance();
    } else if (current_tok.cat == sCHR) {
        tipo = TIPO_CHR;
        advance();
    } else {
        diag_error_syntax(current_tok.line, "sINT, sLOGIC ou sCHR", tokenCatNome(current_tok.cat));
        tipo = TIPO_INT; /* valor arbitrário só para permitir a análise prosseguir */
    }

    if (extra_out != NULL) {
        *extra_out = 0;
    }

    if (current_tok.cat == sABRECOL) {
        int tamanho = 0;
        check(sABRECOL);
        if (current_tok.cat == sCTEINT) {
            /* Token só guarda o lexema como texto; convertemos aqui para
               gravar o tamanho do vetor no campo "extra" da TS. */
            tamanho = atoi(current_tok.lexeme);
        }
        check(sCTEINT);
        check(sFECHACOL);
        if (extra_out != NULL) {
            *extra_out = tamanho;
        }
    }

    return tipo;
}

/* <id> ::= "sIDENTIF" */
static void parse_id(void) {
    check(sIDENTIF);
}

/* <param> ::= ["sREF"] <id> ":" <type> {"," ["sREF"] <id> ":" <type>}
   Retorna a quantidade de parâmetros lidos — usado como "extra" ao
   inserir a própria sub-rotina no escopo externo (ver parse_func e
   parse_proc). Cada parâmetro é inserido no escopo ATUAL, que já deve
   ser o escopo próprio da sub-rotina, aberto por quem chamou esta função.
   Observação: "sREF" é consumido mas não é gravado em lugar nenhum, pois
   o struct Simbolo não tem campo para "passado por referência". */
static int parse_param(void) {
    int count = 0;
    char nome[32];
    SimboloTipo tipo;
    int extra;

    if (current_tok.cat == sREF) {
        check(sREF);
    }
    strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    parse_id();
    check(sDOISPONTOS);
    tipo = parse_type(&extra);
    if (!symtab_insert(nome, CAT_PARAM, tipo, extra)) {
        diag_error_syntax(current_tok.line, "identificador de parametro nao duplicado", nome);
    }
    count++;

    while (current_tok.cat == sVIRG) {
        check(sVIRG);
        if (current_tok.cat == sREF) {
            check(sREF);
        }
        strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
        nome[sizeof(nome) - 1] = '\0';
        parse_id();
        check(sDOISPONTOS);
        tipo = parse_type(&extra);
        if (!symtab_insert(nome, CAT_PARAM, tipo, extra)) {
            diag_error_syntax(current_tok.line, "identificador de parametro nao duplicado", nome);
        }
        count++;
    }

    return count;
}

/* <func> ::= "sFUNC" <id> "(" [<param>] ")" ":" <type> [<lvars>] <bco>
   Quem controla a abertura/fechamento do escopo da função é este parser
   (responsabilidade descrita no enunciado). O símbolo da própria função
   só é inserido no escopo externo DEPOIS de fechar o escopo da função —
   ver a nota "sobre a Tabela de Símbolos" no cabeçalho do arquivo, que
   explica por que essa ordem é necessária e qual a limitação dela para
   chamadas recursivas. */
static void parse_func(void) {
    char nome[32];
    SimboloTipo tipoRet;
    int nparams = 0;

    if (saw_main) {
        diag_error(current_tok.line, "main deve ser a ultima sub-rotina do programa");
    }

    check(sFUNC);
    strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    parse_id();

    if (!symtab_insert(nome, CAT_FUNCAO, TIPO_VOID, 0)) {
        diag_error(current_tok.line, "identificador de funcao nao duplicado");
    }
    symtab_enter_scope(nome);

    check(sABREPAR);
    if (current_tok.cat != sFECHAPAR) {
        nparams = parse_param();
    }
    check(sFECHAPAR);
    check(sDOISPONTOS);
    tipoRet = parse_type(NULL); /* retorno de função nunca é vetor */

    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_block();

    symtab_leave_scope();

    symtab_update_signature(nome, tipoRet, nparams);
}

/* <proc> ::= "sPROC" <id> "(" [<param>] ")" [<lvars>] <bco>
   (mesma lógica de escopo/inserção de parse_func, sem tipo de retorno) */
static void parse_proc(void) {
    char nome[32];
    int nparams = 0;

    check(sPROC);
    strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    parse_id();

    if (strcmp(nome, "main") == 0) {
        if (saw_main) diag_error(current_tok.line, "procedimento main duplicado");
        saw_main = 1;
    } else if (saw_main) {
        diag_error(current_tok.line, "main deve ser a ultima sub-rotina do programa");
    }

    if (!symtab_insert(nome, CAT_PROCEDIMENTO, TIPO_VOID, 0)) {
        diag_error(current_tok.line, "identificador de procedimento nao duplicado");
    }
    symtab_enter_scope(nome);

    check(sABREPAR);
    if (current_tok.cat != sFECHAPAR) {
        nparams = parse_param();
    }
    if (strcmp(nome, "main") == 0 && nparams != 0) {
        diag_error(current_tok.line, "proc main nao pode receber parametros");
    }
    check(sFECHAPAR);
    if (current_tok.cat == sLOCVARS) {
        parse_lvars();
    }
    parse_block();

    symtab_leave_scope();

    symtab_update_signature(nome, TIPO_VOID, nparams);
}

/* <subs> ::= (<func> | <proc>) */
static void parse_subs(void) {
    if (current_tok.cat == sFUNC) {
        parse_func();
    } else if (current_tok.cat == sPROC) {
        parse_proc();
    } else {
        diag_error_syntax(current_tok.line, "sFUNC ou sPROC", tokenCatNome(current_tok.cat));
    }
}

/* <bco> ::= "sSTART" {<cmd> ";"} "sEND"
   Não abrimos um escopo de symtab aqui: nessa gramática só existe um
   <bco> por sub-rotina (o corpo dela), e nenhum <cmd> dentro do bloco
   declara identificadores novos — todas as declarações (parâmetros e
   locvars) já acontecem antes do <bco>, no escopo aberto por parse_func/
   parse_proc. Um escopo extra aqui ficaria sempre vazio. */
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
    char nome[32];
    int line = current_tok.line;
    strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    parse_use_id();
    {
        Simbolo *s = symtab_lookup(nome);
        if (s && s->extra <= 0) {
            diag_error(line, "identificador nao e um vetor");
        }
    }
    check(sABRECOL);
    if (current_tok.cat == sCTEINT) {
        advance();
    } else if (current_tok.cat == sIDENTIF) {
        parse_use_id();
    } else {
        diag_error_syntax(current_tok.line, "sCTEINT ou sIDENTIF", tokenCatNome(current_tok.cat));
    }
    check(sFECHACOL);
}

/* <call> ::= <id> "(" [<expr> {"," <expr>}] ")" */
static void parse_call(void) {
    char nome[32];
    int line = current_tok.line;
    strncpy(nome, current_tok.lexeme, sizeof(nome) - 1);
    nome[sizeof(nome) - 1] = '\0';
    parse_id();

    {
        Simbolo *s = symtab_lookup(nome);
        if (!s || (s->cat != CAT_FUNCAO && s->cat != CAT_PROCEDIMENTO)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "sub-rotina '%s' nao declarada", nome);
            diag_error(line, msg);
        }
    }
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
        parse_use_id();
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
        parse_use_id();
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
    parse_use_id();
    check(sFROM);
    if (current_tok.cat == sIDENTIF || current_tok.cat == sCTEINT) {
        if (current_tok.cat == sIDENTIF) parse_use_id(); else advance();
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF ou sCTEINT", tokenCatNome(current_tok.cat));
    }
    check(sTO);
    if (current_tok.cat == sIDENTIF || current_tok.cat == sCTEINT) {
        if (current_tok.cat == sIDENTIF) parse_use_id(); else advance();
    } else {
        diag_error_syntax(current_tok.line, "sIDENTIF ou sCTEINT", tokenCatNome(current_tok.cat));
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
            diag_error_syntax(current_tok.line, "comando valido", tokenCatNome(current_tok.cat));
            break;
    }
}

/* <elem> ::= <litl> | <id> | <vetr> | <call>
   Observação: a alternativa <expr> listada para <elem> no apêndice é
   redundante/inacessível aqui — expressões entre parênteses já são
   tratadas em <fact> como "(" <expr> ")"; se <elem> também aceitasse
   <expr> diretamente teríamos recursão sem consumo de token. */
static void parse_atom(void) {
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
            parse_use_id();
        }
        return;
    }

    diag_error_syntax(current_tok.line, "elemento valido (literal, identificador, vetor ou chamada)", tokenCatNome(current_tok.cat));
}

/* A produção elem da especificação também admite expressões completas. */
static void parse_elem(void) {
    parse_expr();
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
        parse_atom();
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
   (ver nota no cabeçalho sobre <princ> não ser tratado separadamente aqui)

   O escopo "global" é aberto/fechado aqui porque é o parser quem
   controla escopos, e esta é a única produção que enxerga o programa
   inteiro. symtab_init()/symtab_destroy() continuam sendo chamados pelo
   main, fora do parser. */
void parse_program(void) {
    advance();
    saw_main = 0;

    symtab_enter_scope("global");

    if (current_tok.cat == sGLOBVARS) {
        parse_gvars();
    }

    while (current_tok.cat == sFUNC || current_tok.cat == sPROC) {
        parse_subs();
    }

    if (!saw_main) {
        diag_error(current_tok.line, "programa deve conter o procedimento proc main()");
    }

    symtab_leave_scope();

    check(TOKEN_EOF); /* Garante término do arquivo */
}
