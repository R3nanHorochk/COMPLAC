# COMPLAC

COMPLAC é um compilador para a linguagem SLAC², implementado em C e estruturado em torno de um analisador léxico, tabela de símbolos e parser descendente recursivo.

## Requisitos

- GCC ou outro compilador compatível com C99
- Ambiente Unix/Linux (o projeto usa `make` e a linha de comando do shell)

## Compilação

No diretório raiz do projeto, execute:

```bash
make
```

Isso gera o executável `complac` na pasta atual.

Para remover artefatos de compilação:

```bash
make clean
```

## Execução

```bash
./complac <arquivo.slac> [--tokens] [--symtab] [--trace]
```

Exemplo:

```bash
./complac teste.slac --tokens --symtab --trace
```

### Opções da linha de comando

- `--tokens`: gera um arquivo `.tk` com a sequência de tokens do programa-fonte
- `--symtab`: gera um arquivo `.ts` com a tabela de símbolos
- `--trace`: gera um arquivo `.trc` com mensagens de rastreamento da execução

Os arquivos de saída são criados com o mesmo nome base do arquivo de entrada, trocando apenas a extensão. Por exemplo, para `teste.slac` podem ser produzidos:

- `teste.tk`
- `teste.ts`
- `teste.trc`

## Observações de implementação

- A função `main()` em `main.c` coordena a inicialização do compilador: argumentos, diagnósticos, logs, lexer, tabela de símbolos e parser.
- O parser usa lookahead de um token para distinguir identificadores em contextos ambíguos, como chamadas de função e atribuições.
- O projeto ainda atua como analisador/validador sintático da linguagem SLAC²; não há geração de código objeto ou assembly neste ponto.
- O fluxo principal é:
  1. leitura da fonte
  2. tokenização
  3. análise sintática
  4. registro de símbolos e diagnósticos
  5. fechamento dos recursos e finalização do programa

## Estrutura principal

- `main.c`: ponto de entrada e orquestração
- `lex.c` / `token.c`: análise léxica e representação dos tokens
- `parser.c`: parser descendente recursivo
- `symtab.c`: tabela de símbolos
- `diag.c`: mensagens de erro e diagnóstico
- `opt.c`: processamento de opções da linha de comando
- `log.c`: geração dos arquivos auxiliares de log

## Arquivo de exemplo

O repositório inclui `teste.slac`, que pode ser usado como entrada para validar o comportamento básico do compilador.

```bash
./complac teste.slac --tokens --symtab --trace
```
