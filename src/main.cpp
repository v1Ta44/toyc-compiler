#include "compiler.hpp"
#include "parser.hpp"
#include <iostream>

// 编译器入口：输入和输出均使用标准流，方便评测系统重定向文件。
int main(int argc, char **argv) {
    try {
        // 评测约定只有一个可选开关 -opt；未传入时仍执行常量折叠。
        bool opt = argc > 1 && std::string(argv[1]) == "-opt";
        // Bison C++ parser 从 Flex 提供的 yylex 中逐个读取 token。
        yy::parser p;
        if (p.parse() != 0 || !toyc::g_program)
            return 1;
        // 语法分析成功后，后端负责语义相关检查、IR 生成和汇编输出。
        toyc::compile(*toyc::g_program, std::cout, opt);
        delete toyc::g_program;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
