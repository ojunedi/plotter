#pragma once

#include <memory>
#include <variant>
#include <string>

// forward declare
struct Expr;
using Box = std::unique_ptr<Expr>;

struct Number { float value; };
struct BinOp { char op; Box left, right; };
struct Variable {}; // always x for now
struct FuncCall { std::string name; Box arg; };
struct UnaryMinus { Box operand; };

struct Expr : std::variant<Number, Variable, BinOp, UnaryMinus, FuncCall> {
    using variant::variant;
};
