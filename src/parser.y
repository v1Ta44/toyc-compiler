%language "c++"
/* 使用 Bison C++ 接口，并用 variant 承载不同非终结符的语义值。 */
%define api.value.type variant
%define parse.error verbose

%code requires {
#include "compiler.hpp"
}

%code top {
#include "compiler.hpp"
using namespace toyc;
}

%code {
#include "compiler.hpp"
}

%code provides {
int yylex(yy::parser::semantic_type *);
}

%token <int> NUMBER
%token <std::string *> ID
%token CONST INT VOID IF ELSE WHILE BREAK CONTINUE RETURN
%token OROR ANDAND LE GE EQ NE

%left OROR
%left ANDAND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UPLUS UMINUS '!'

%expect 1
%start program

%type <toyc::Program *> program
%type <toyc::Function *> function
%type <toyc::Stmt *> stmt block
%type <toyc::Expr *> expr
%type <std::vector<toyc::Stmt *> *> stmts
%type <std::vector<std::string>> params paramlist
%type <std::vector<std::unique_ptr<toyc::Expr>>> args

%%

/* 编译单元由若干全局声明或函数定义组成。 */
program:
    items {
        $$ = g_program;
    }
;

items:
    %empty
  | items top
;

top:
    decl {
        if (!g_program)
            g_program = new Program;
    }
  | function {
        if (!g_program)
            g_program = new Program;
        g_program->funcs.emplace_back($1);
    }
;

decl:
    /* ToyC 要求所有变量和常量声明都带初始化表达式。 */
    CONST INT ID '=' expr ';' {
        if (!g_program)
            g_program = new Program;
        Decl declaration{*$3, true, std::unique_ptr<Expr>($5)};
        g_program->globals.push_back(std::move(declaration));
        delete $3;
    }
  | INT ID '=' expr ';' {
        if (!g_program)
            g_program = new Program;
        Decl declaration{*$2, false, std::unique_ptr<Expr>($4)};
        g_program->globals.push_back(std::move(declaration));
        delete $2;
    }
;

function:
    INT ID '(' params ')' block {
        $$ = new Function{*$2, "int", $4, std::move($6->body)};
        delete $2;
        delete $6;
    }
  | VOID ID '(' params ')' block {
        $$ = new Function{*$2, "void", $4, std::move($6->body)};
        delete $2;
        delete $6;
    }
;

params:
    %empty {
        $$ = std::vector<std::string>{};
    }
  | paramlist {
        $$ = $1;
    }
;

paramlist:
    INT ID {
        $$ = std::vector<std::string>{*$2};
        delete $2;
    }
  | paramlist ',' INT ID {
        $1.push_back(*$4);
        delete $4;
        $$ = $1;
    }
;

block:
    '{' stmts '}' {
        $$ = new Stmt(Stmt::Block);
        for (auto statement : *$2)
            $$->body.emplace_back(statement);
        delete $2;
    }
;

stmts:
    %empty {
        $$ = new std::vector<Stmt *>;
    }
  | stmts stmt {
        $1->push_back($2);
        $$ = $1;
    }
;

stmt:
    /* dangling-else 冲突按 Bison 默认的移进规则绑定到最近的 if。 */
    ';' {
        $$ = new Stmt(Stmt::Empty);
    }
  | expr ';' {
        $$ = new Stmt(Stmt::ExprStmt);
        $$->expr.reset($1);
    }
  | INT ID '=' expr ';' {
        $$ = new Stmt(Stmt::Decl);
        $$->name = *$2;
        $$->expr.reset($4);
        delete $2;
    }
  | CONST INT ID '=' expr ';' {
        $$ = new Stmt(Stmt::Decl);
        $$->name = *$3;
        $$->isConst = true;
        $$->expr.reset($5);
        delete $3;
    }
  | ID '=' expr ';' {
        $$ = new Stmt(Stmt::Assign);
        $$->name = *$1;
        $$->expr.reset($3);
        delete $1;
    }
  | block {
        $$ = $1;
    }
  | IF '(' expr ')' stmt {
        $$ = new Stmt(Stmt::If);
        $$->cond.reset($3);
        $$->thenS.reset($5);
    }
  | IF '(' expr ')' stmt ELSE stmt {
        $$ = new Stmt(Stmt::If);
        $$->cond.reset($3);
        $$->thenS.reset($5);
        $$->elseS.reset($7);
    }
  | WHILE '(' expr ')' stmt {
        $$ = new Stmt(Stmt::While);
        $$->cond.reset($3);
        $$->thenS.reset($5);
    }
  | BREAK ';' {
        $$ = new Stmt(Stmt::Break);
    }
  | CONTINUE ';' {
        $$ = new Stmt(Stmt::Continue);
    }
  | RETURN ';' {
        $$ = new Stmt(Stmt::Return);
    }
  | RETURN expr ';' {
        $$ = new Stmt(Stmt::Return);
        $$->expr.reset($2);
    }
;

expr:
    /* 运算符优先级和结合性由文件头部的 %left/%right 声明决定。 */
    NUMBER {
        $$ = new Expr(Expr::Num);
        $$->value = $1;
    }
  | ID {
        $$ = new Expr(Expr::Var);
        $$->name = *$1;
        delete $1;
    }
  | ID '(' args ')' {
        $$ = new Expr(Expr::Call);
        $$->name = *$1;
        $$->args = std::move($3);
        delete $1;
    }
  | '(' expr ')' {
        $$ = $2;
    }
  | '+' expr %prec UPLUS {
        $$ = new Expr(Expr::Unary);
        $$->op = "+";
        $$->a.reset($2);
    }
  | '-' expr %prec UMINUS {
        $$ = new Expr(Expr::Unary);
        $$->op = "-";
        $$->a.reset($2);
    }
  | '!' expr {
        $$ = new Expr(Expr::Unary);
        $$->op = "!";
        $$->a.reset($2);
    }
  | expr '+' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "+";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '-' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "-";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '*' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "*";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '/' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "/";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '%' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "%";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '<' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "<";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr '>' expr {
        $$ = new Expr(Expr::Binary);
        $$->op = ">";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr LE expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "<=";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr GE expr {
        $$ = new Expr(Expr::Binary);
        $$->op = ">=";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr EQ expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "==";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr NE expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "!=";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr ANDAND expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "&&";
        $$->a.reset($1);
        $$->b.reset($3);
    }
  | expr OROR expr {
        $$ = new Expr(Expr::Binary);
        $$->op = "||";
        $$->a.reset($1);
        $$->b.reset($3);
    }
;

args:
    /* 空参数列表和逗号分隔的实参列表分别对应两种调用形式。 */
    %empty {
        $$ = std::vector<std::unique_ptr<Expr>>{};
    }
  | expr {
        $$ = std::vector<std::unique_ptr<Expr>>{};
        $$.emplace_back($1);
    }
  | args ',' expr {
        $1.emplace_back($3);
        $$ = std::move($1);
    }
;

%%

void yy::parser::error(const std::string &s) {
}
