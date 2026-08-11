%option noyywrap yylineno nounput noinput
%{
/* Flex 生成的扫描器把 token 和语义值交给 Bison C++ parser。 */
#include "parser.hpp"
#include <cstdlib>
#define YY_DECL int yylex(yy::parser::semantic_type* yylval)
%}
ID [_A-Za-z][_A-Za-z0-9]*
/* 负号单独作为运算符处理，避免 n-1 被最长匹配成 ID 和 -1 两个错误 token。 */
NUM (0|[1-9][0-9]*)
%%
 /* 空白和 C/C++ 风格注释不进入语法树。 */
[ \t\r\n]+                   ;
"//"[^\n]*                   ;
"/*"([^*]|\*+[^*/])*"*/"     ;
"const"                      return yy::parser::token::CONST;
"int"                        return yy::parser::token::INT;
"void"                       return yy::parser::token::VOID;
"if"                         return yy::parser::token::IF;
"else"                       return yy::parser::token::ELSE;
"while"                      return yy::parser::token::WHILE;
"break"                      return yy::parser::token::BREAK;
"continue"                   return yy::parser::token::CONTINUE;
"return"                     return yy::parser::token::RETURN;
"||"                         return yy::parser::token::OROR;
"&&"                         return yy::parser::token::ANDAND;
"<="                         return yy::parser::token::LE;
">="                         return yy::parser::token::GE;
"=="                         return yy::parser::token::EQ;
"!="                         return yy::parser::token::NE;
{NUM}                         {
    /* variant 语义值必须显式构造为 int。 */
    yylval->emplace<int>(std::strtol(yytext, nullptr, 10));
    return yy::parser::token::NUMBER;
}
{ID}                          {
    /* ID 使用堆上的 string 传递，Bison 归约后负责释放它。 */
    yylval->emplace<std::string *>(new std::string(yytext));
    return yy::parser::token::ID;
}
.                            return yytext[0];
%%
