#include "parser.hpp"
#include "eval.hpp"
#include <iostream>

int main() {
    std::string expr_str = "log10(x)";
    auto tokens = lex(expr_str);
    Parser p{tokens, 0};
    Expr expr = p.Parse();

    std::cout << "AST: " << expr << "\n";

    for (float x : {0.0f, 1.0f, 2.0f, 3.14f}) {
        std::cout << "f(" << x << ") = " << eval(expr, x) << "\n";
    }
}
