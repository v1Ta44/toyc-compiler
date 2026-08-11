// ToyC 前端 AST、程序模型以及编译器对外接口。
#pragma once
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
namespace toyc {
// 表达式节点。a/b 表示一元或二元运算的子表达式，args 保存函数实参。
struct Expr {
    enum Kind { Num, Var, Call, Unary, Binary };
    Kind kind;
    int value{};
    std::string name, op;
    std::vector<std::unique_ptr<Expr>> args;
    std::unique_ptr<Expr> a, b;
    Expr(Kind k) : kind(k) {
    }
};
// 语句节点。Block 的 body 是有序语句列表，If/While 使用 thenS/elseS 保存子语句。
struct Stmt {
    enum Kind { Empty, ExprStmt, Decl, Assign, Block, If, While, Break, Continue, Return };
    Kind kind;
    std::string name;
    bool isConst{};
    std::unique_ptr<Expr> expr, cond;
    std::vector<std::unique_ptr<Stmt>> body;
    std::unique_ptr<Stmt> thenS, elseS;
    Stmt(Kind k) : kind(k) {
    }
};
// 全局声明和块内声明共用的简单描述；局部声明在语法树中由 Stmt::Decl 表示。
struct Decl {
    std::string name;
    bool isConst{};
    std::unique_ptr<Expr> init;
};
// 函数定义，包括返回类型、形参名和函数体。
struct Function {
    std::string name, ret;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Stmt>> body;
};
// 编译单元根节点。ToyC 的全局声明和函数定义都只能出现在这里。
struct Program {
    std::vector<Decl> globals;
    std::vector<std::unique_ptr<Function>> funcs;
};
// 将 AST 降低为自定义 IR，并把 IR 输出为 RV32 汇编。
void compile(Program &, std::ostream &, bool optimize);
// Bison 语义动作构造的全局程序对象。
extern Program *g_program;
} // namespace toyc
