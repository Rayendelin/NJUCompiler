%locations
%define parse.error verbose

%{
    #include <stdarg.h>
    #include "lex.yy.c"
    Node* root = NULL;
    Node** package(int childNum, Node* child1, ...);
    void yyerror(const char* msg);
    int synError = 0;
%}

%token INT FLOAT ID SEMI COMMA ASSIGNOP RELOP 
%token PLUS MINUS STAR DIV AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
/* High-level Definitions */
Program : ExtDefList                            { $$ = createNode("Program", ENUM_SYN_NOT_NULL, @$.first_line, 
                                                  1, package(1, $1));
                                                  root = $$; }
    ;
ExtDefList : ExtDef ExtDefList                  { $$ = createNode("ExtDefList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("ExtDefList", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL);}
    ;
ExtDef : Specifier ExtDecList SEMI              { $$ = createNode("ExtDef", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier SEMI                            { $$ = createNode("ExtDef", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | Specifier FunDec SEMI                     { $$ = createNode("ExtDef", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier FunDec CompSt                   { $$ = createNode("ExtDef", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Specifier error SEMI                      { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | error SEMI                                { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Specifier error                           { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
ExtDecList : VarDec                             { $$ = createNode("ExtDecList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec COMMA ExtDecList                   { $$ = createNode("ExtDecList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | VarDec error COMMA ExtDecList             { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;

/* Specifiers */
Specifier : TYPE                                { $$ = createNode("Specifier", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | StructSpecifier                           { $$ = createNode("Specifier", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = createNode("StructSpecifier", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | STRUCT Tag                                { $$ = createNode("StructSpecifier", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | STRUCT error LC DefList RC                { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | STRUCT OptTag LC error RC                 { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | STRUCT OptTag LC error                    { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | STRUCT error                              { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
OptTag : ID                                     { $$ = createNode("OptTag", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | /* empty */                               { $$ = createNode("OptTag", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Tag : ID                                        { $$ = createNode("Tag", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;

/* Declarators */
VarDec : ID                                     { $$ = createNode("VarDec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec LB INT RB                          { $$ = createNode("VarDec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | VarDec LB error RB                        { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | VarDec LB error                           { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
FunDec : ID LP VarList RP                       { $$ = createNode("FunDec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | ID LP RP                                  { $$ = createNode("FunDec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ID LP error RP                            { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | ID LP error                               { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
VarList : ParamDec COMMA VarList                { $$ = createNode("VarList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ParamDec                                  { $$ = createNode("VarList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
ParamDec : Specifier VarDec                     { $$ = createNode("ParamDec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    ;

/* Statements */
CompSt : LC DefList StmtList RC                 { $$ = createNode("CompSt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    ;
StmtList : Stmt StmtList                        { $$ = createNode("StmtList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("StmtList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Stmt : Exp SEMI                                 { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | CompSt                                    { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | RETURN Exp SEMI                           { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | IF LP Exp RP Stmt ELSE Stmt               { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 7, package(7, $1, $2, $3, $4, $5, $6, $7)); }
    | WHILE LP Exp RP Stmt                      { $$ = createNode("Stmt", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 5, package(5, $1, $2, $3, $4, $5)); }
    | error SEMI                                { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP Exp RP error ELSE Stmt              { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | IF LP error RP ELSE Stmt              { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | error LP Exp RP Stmt                      { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;

/* Local Definitions */
DefList : Def DefList                           { $$ = createNode("DefList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | /* empty */                               { $$ = createNode("Stmt", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); }
    ;
Def : Specifier DecList SEMI                    { $$ = createNode("Def", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    ;
DecList : Dec                                   { $$ = createNode("DecList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Dec COMMA DecList                         { $$ = createNode("DecList", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Dec error DecList                         { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Dec : VarDec                                    { $$ = createNode("Dec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | VarDec ASSIGNOP Exp                       { $$ = createNode("Dec", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | error ASSIGNOP Exp                        { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp                          { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp AND Exp                               { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp OR Exp                                { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp RELOP Exp                             { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp PLUS Exp                              { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp MINUS Exp                             { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp STAR Exp                              { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp DIV Exp                               { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | LP Exp RP                                 { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | MINUS Exp                                 { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | NOT Exp                                   { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 2, package(2, $1, $2)); }
    | ID LP Args RP                             { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | ID LP RP                                  { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp LB Exp RB                             { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 4, package(4, $1, $2, $3, $4)); }
    | Exp DOT ID                                { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | ID                                        { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | INT                                       { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | FLOAT                                     { $$ = createNode("Exp", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    | Exp ASSIGNOP error                        { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp AND error                             { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp OR error                              { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp RELOP error                           { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp PLUS error                            { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp MINUS error                           { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp STAR error                            { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp DIV error                             { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | ID LP error RP                            { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    | Exp LB error RB                           { $$ = createNode("Error", ENUM_SYN_NULL, @$.first_line
                                                  , 0, NULL); yyerrok; }
    ;
Args : Exp COMMA Args                           { $$ = createNode("Args", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 3, package(3, $1, $2, $3)); }
    | Exp                                       { $$ = createNode("Args", ENUM_SYN_NOT_NULL, @$.first_line
                                                  , 1, package(1, $1)); }
    ;
%%
Node** package(int childNum, Node* child1, ...) {
    va_list ap;
    va_start(ap, child1);
    Node** res = (Node**)malloc(sizeof(Node*) * childNum);
    res[0] = child1;
    for (int i = 1; i < childNum; i++)
    {
        res[i] = va_arg(ap, Node*);
    }
    return res;
}

void yyerror(const char* msg) { 
    synError++;
    printf("Error type B at Line %d: %s\n", yylineno, msg);
}