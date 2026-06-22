#pragma once
#include <stdexcept>
#include <variant>
#include <cmath>
#include <unordered_map>
#include "ast.hpp"
#include <lexer.hpp>

const std::unordered_map<std::string, float(*)(float)> functions = {
    {"sin",  sinf},
    {"cos",  cosf},
    {"tan",  tanf},
    {"asin", asinf},
    {"acos", acosf},
    {"atan", atanf},
    {"sinh", sinhf},
    {"cosh", coshf},
    {"tanh", tanhf},
    {"exp",  expf},
    {"log",  logf},
    {"ln",   logf},
    {"log10",log10f},
    {"sqrt", sqrtf},
    {"abs",  fabsf},
    {"ceil", ceilf},
    {"floor",floorf},
};

inline double eval(const Expr &expr, double x) {
    // walk the ast replacing Variable with x
    // evaluate
    return std::visit([&x](auto &&node) -> double {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Number>) {
            return node.value;
        } else if constexpr (std::is_same_v<T, Variable>) {
            return x;
        } else if constexpr (std::is_same_v<T, UnaryMinus>) {
            return -eval(*node.operand, x);
        } else if constexpr (std::is_same_v<T, BinOp>) {
            double left = eval(*node.left, x);
            double right = eval(*node.right, x);
            switch (node.op) {
                case TokenType::Plus:  return left + right;
                case TokenType::Minus: return left - right;
                case TokenType::Star:  return left * right;
                case TokenType::Slash: return left / right;
                case TokenType::Caret: return pow(left, right);
                default: throw std::runtime_error("Eval: unexpected TokenType in binary operation");
            }
        } else if constexpr (std::is_same_v<T, FuncCall>) {
            auto it = functions.find(node.name);
            if (it == functions.end()) throw std::runtime_error("unknown function: " + node.name);
            return it->second(eval(*node.arg, x));
        }
        throw std::runtime_error("eval: unhandled node type");
    }, expr);
}
