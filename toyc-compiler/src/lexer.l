%{
#include "ast/ast.hpp"
#include "parser.hpp"
#include <string>

extern int yylineno;
%}

%option noyywrap
%option yylineno

/* 正则表达式定义 */
DIGIT       [0-9]
LETTER      [a-zA-Z_]
ID          {LETTER}({LETTER}|{DIGIT})*
NUMBER      -?(0|[1-9]{DIGIT}*)
WHITESPACE  [ \t\r]+
NEWLINE     \n

%%

{WHITESPACE}    { /* 忽略空白字符 */ }
{NEWLINE}       { /* 忽略换行符 */ }

"//".*          { /* 忽略单行注释 */ }
"/*"            { 
                    int c;
                    while ((c = yyinput()) != 0) {
                        if (c == '*') {
                            if ((c = yyinput()) == '/') {
                                break;
                            }
                            unput(c);
                        }
                    }
                }

/* 关键字 */
"int"           { return INT; }
"void"          { return VOID; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }
"return"        { return RETURN; }

/* 运算符 */
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MULTIPLY; }
"/"             { return DIVIDE; }
"%"             { return MOD; }
"="             { return ASSIGN; }
"=="            { return EQ; }
"!="            { return NE; }
"<"             { return LT; }
"<="            { return LE; }
">"             { return GT; }
">="            { return GE; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }

/* 分隔符 */
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
","             { return COMMA; }
";"             { return SEMICOLON; }

/* 标识符和数字 */
{ID}            { 
                    yylval.string_val = new std::string(yytext);
                    return IDENTIFIER; 
                }
{NUMBER}        { 
                    yylval.int_val = atoi(yytext);
                    return NUMBER; 
                }

/* 未知字符 */
.               { 
                    fprintf(stderr, "Unknown character: %s at line %d\n", yytext, yylineno);
                    return yytext[0]; 
                }

%%