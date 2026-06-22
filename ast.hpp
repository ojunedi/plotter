#pragma once

#include <lexer.hpp>
#include <memory>
#include <ostream>
#include <string>
#include <variant>

// forward declare
struct Expr;
using Box = std::unique_ptr<Expr>;

struct Number { double value; };
struct BinOp { TokenType op; Box left, right; };
struct Variable {}; // always x for now
struct FuncCall { std::string name; Box arg; };
struct UnaryMinus { Box operand; };

struct Expr : std::variant<Number, Variable, BinOp, UnaryMinus, FuncCall> {
    using variant::variant;
};

inline std::ostream &operator<<(std::ostream &os, const Expr &e) {
    std::visit([&os](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Number>) {
            os << "Number{" << node.value << "}";
        } else if constexpr (std::is_same_v<T, Variable>) {
            os << "Variable{}";
        } else if constexpr (std::is_same_v<T, UnaryMinus>) {
            os << "UnaryMinus{" << *node.operand << "}";
        } else if constexpr (std::is_same_v<T, BinOp>) {
            char op;
            switch (node.op) {
                case TokenType::Plus:  op = '+'; break;
                case TokenType::Minus: op = '-'; break;
                case TokenType::Star:  op = '*'; break;
                case TokenType::Slash: op = '/'; break;
                case TokenType::Caret: op = '^'; break;
                default:               op = '?'; break;
            }
            os << "BinOp{" << op << ", " << *node.left << ", " << *node.right << "}";
        } else if constexpr (std::is_same_v<T, FuncCall>) {
            os << "FuncCall{" << node.name << ", " << *node.arg << "}";
        }
    }, e);
    return os;
}
