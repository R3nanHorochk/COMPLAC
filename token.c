#include "token.h"

const char *token_cat_name(TokenCategoria cat) {
    switch (cat) {
        case TOKEN_EOF:      return "EOF";
        case TOKEN_ERROR:    return "ERROR";

        /* Categorias básicas e literais */
        case sIDENTIF:       return "sIDENTIF";
        case sCTEINT:        return "sCTEINT";
        case sCTECHAR:       return "sCTECHAR";
        case sSTRING:        return "sSTRING";

        /* Palavras reservadas */
        case sGLOBVARS:      return "sGLOBVARS";
        case sLOCVARS:       return "sLOCVARS";
        case sKIND:          return "sKIND";
        case sINT:           return "sINT";
        case sLOGIC:         return "sLOGIC";
        case sCHR:           return "sCHR";
        case sFUNC:          return "sFUNC";
        case sPROC:          return "sPROC";
        case sREF:           return "sREF";
        case sSTART:         return "sSTART";
        case sEND:           return "sEND";
        case sECHO:          return "sECHO";
        case sGET:           return "sGET";
        case sCASE:          return "sCASE";
        case sOTHERWISE:     return "sOTHERWISE";
        case sCHOOSE:        return "sCHOOSE";
        case sMATCH:         return "sMATCH";
        case sOTHERS:        return "sOTHERS";
        case sFOR:           return "sFOR";
        case sFROM:          return "sFROM";
        case sTO:            return "sTO";
        case sBY:            return "sBY";
        case sDO:            return "sDO";
        case sWHILE:         return "sWHILE";
        case sREPEAT:        return "sREPEAT";
        case sUNTIL:         return "sUNTIL";
        case sRETURN:        return "sRETURN";

        /* Operadores e delimitadores */
        case sATRIB:         return "sATRIB";
        case sIMPLIC:        return "sIMPLIC";
        case sSOMA:          return "sSOMA";
        case sSUBRAT:        return "sSUBRAT";
        case sMULT:          return "sMULT";
        case sDIV:           return "sDIV";
        case sMAIOR:         return "sMAIOR";
        case sMAIORIGUAL:    return "sMAIORIGUAL";
        case sMENOR:         return "sMENOR";
        case sMENORIGUAL:    return "sMENORIGUAL";
        case sIGUAL:         return "sIGUAL";
        case sDIFERENTE:     return "sDIFERENTE";
        case sAND:           return "sAND";
        case sOR:            return "sOR";
        case sNEG:           return "sNEG";
        case sABREPAR:       return "sABREPAR";
        case sFECHAPAR:      return "sFECHAPAR";
        case sABRECOL:       return "sABRECOL";
        case sFECHACOL:      return "sFECHACOL";
        case sPONTOVIRG:     return "sPONTOVIRG";
        case sVIRG:          return "sVIRG";
        case sDOISPONTOS:    return "sDOISPONTOS";

        default:             return "sDESCONHECIDO";
    }
}