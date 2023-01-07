%option yylineno

%{
    #include <stdlib.h>
    #include <ctype.h>
    #include "Tree.h"
    #define YYSTYPE Node*
    #include "syntax.tab.h"
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
    int lexError = 0;
    int charToi(char ch);
    int hexstrToi(char* text);
    int octstrToi(char* text);
%}

digit       [0-9]
letter      [_a-zA-Z]
DEC         0|[1-9]{digit}*
OCT         0[0-7]+
HEX         (0x|0X)[0-9a-fA-F]+
INT         {DEC}|{OCT}|{HEX}
FLOAT       (({digit}*\.{digit}+|{digit}+\.)[eE][+-]?{digit}+)|({digit}+\.{digit}+)
ID          {letter}+({digit}|{letter})*
RELOP       ">"|"<"|">="|"<="|"=="|"!="
TYPE        "int"|"float"
SPACE       [ \t\r]+
%%
"\n"        { yycolumn = 1; }
{SPACE}     { }
"//"        { char c = input(); 
              while (c != '\n') c = input(); }
"/*"        { char a = input();
              char b = input();
              while (!(a == '*' && b == '/')) { a = b; b = input(); } }
";"         { yylval = createNode("SEMI", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return SEMI; }
","         { yylval = createNode("COMMA", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return COMMA; }
"="         { yylval = createNode("ASSIGNOP", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return ASSIGNOP; }
"+"         { yylval = createNode("PLUS", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return PLUS; }
"-"         { yylval = createNode("MINUS", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return MINUS; }
"*"         { yylval = createNode("STAR", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return STAR; }
"/"         { yylval = createNode("DIV", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return DIV; }
"&&"        { yylval = createNode("AND", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return AND; }
"||"        { yylval = createNode("OR", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return OR; }
"."         { yylval = createNode("DOT", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return DOT; }
"!"         { yylval = createNode("NOT", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return NOT; }
"("         { yylval = createNode("LP", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return LP; }
")"         { yylval = createNode("RP", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return RP; }
"["         { yylval = createNode("LB", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return LB; }
"]"         { yylval = createNode("RB", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return RB; }
"{"         { yylval = createNode("LC", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return LC; }
"}"         { yylval = createNode("RC", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return RC; }
"struct"    { yylval = createNode("STRUCT", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return STRUCT; }
"return"    { yylval = createNode("RETURN", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return RETURN; }
"if"        { yylval = createNode("IF", ENUM_LEX_OTHER, yylineno, 0, NULL); 
              return IF; }
"else"      { yylval = createNode("ELSE", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return ELSE; }
"while"     { yylval = createNode("WHILE", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return WHILE; }
{RELOP}     { yylval = createNode("RELOP", ENUM_LEX_OTHER, yylineno, 0, NULL);
              return RELOP; }
{TYPE}      { yylval = createNode("TYPE", ENUM_LEX_TYPE, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return TYPE; }
{HEX}       { yylval = createNode("INT", ENUM_LEX_INT, yylineno, 0, NULL);
              yylval->intVal = hexstrToi(yytext);
              return INT; }
{OCT}       { yylval = createNode("INT", ENUM_LEX_INT, yylineno, 0, NULL);
              yylval->intVal = octstrToi(yytext);
              return INT; }
{DEC}       { yylval = createNode("INT", ENUM_LEX_INT, yylineno, 0, NULL);
              yylval->intVal = atoi(yytext);
              return INT; }
{FLOAT}     { yylval = createNode("FLOAT", ENUM_LEX_FLOAT, yylineno, 0, NULL);
              yylval->floatVal = atof(yytext);
              return FLOAT; }
{ID}        { yylval = createNode("ID", ENUM_LEX_ID, yylineno, 0, NULL);
              strcpy(yylval->strVal, yytext);
              return ID; }
.           { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); 
              lexError++; }
%%
int charToi(char ch)
{   // 如果是数字，则用数字的ASCII码减去48, 如果ch = '2' ,则 '2' - 48 = 2
    if(isdigit(ch))
        return ch - 48;
    // 如果是字母，但不是A~F,a~f则返回
    if( ch < 'A' || (ch > 'F' && ch < 'a') || ch > 'z' )
        return -1;
    // 如果是大写字母，则用数字的ASCII码减去55, 如果ch = 'A' ,则 'A' - 55 = 10
    // 如果是小写字母，则用数字的ASCII码减去87, 如果ch = 'a' ,则 'a' - 87 = 10
    if(isalpha(ch))
        return isupper(ch) ? ch - 55 : ch - 87;
    return -1;
}

int hexstrToi(char* text) {
    int len = strlen(text);
    int res = 0;
    for (int i = 2; i < len; i++) {
        res *= 16;
        res += charToi(text[i]);
    }
    return res;
}

int octstrToi(char* text) {
    int len = strlen(text);
    int res = 0;
    for (int i = 0; i < len; i++) {
        res *= 8;
        res += charToi(text[i]);
    }
    return res;
}