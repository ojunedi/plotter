#pragma once
#include "ast.hpp"
#include "lexer.hpp"
#include <cmath>
#include <memory>
#include <stdexcept>

struct Parser {
    std::vector<Token> tokens;
    int pos = 0;

    Token peek()    { return tokens[pos]; }
    Token consume() { return tokens[pos++]; }
    Token expect(TokenType t) {
        if (peek().type != t) throw std::runtime_error("unexpected token");
        return consume();
    }

    Expr Parse();
    Expr ParseExpr();
    Expr ParseTerm();
    Expr ParseFactor();
    Expr ParseBase();
};

inline Expr Parser::Parse() {
    Expr result = ParseExpr();
    expect(TokenType::End);
    return result;
}

inline Expr Parser::ParseBase() {
    switch (peek().type) {
    case TokenType::Number: {
        auto v = consume().value;
        return Number{v};
    }
    case TokenType::Ident: {
        auto prev = consume();
        if (peek().type == TokenType::LParen) {
            consume();
            auto arg = ParseExpr();
            expect(TokenType::RParen);
            return FuncCall{prev.text, std::make_unique<Expr>(std::move(arg))};
        } else {
            if (prev.text == "x")   return Variable{};
            if (prev.text == "pi")  return Number{M_PI};
            if (prev.text == "e")   return Number{M_E};
            throw std::runtime_error("unknown identifier: " + prev.text);
        }
    }
    case TokenType::Minus: {
        consume();
        Expr operand = ParseBase();
        return UnaryMinus{std::make_unique<Expr>(std::move(operand))};
    }
    case TokenType::LParen: {
        consume();
        Expr inner = ParseExpr();
        expect(TokenType::RParen);
        return inner;
    }
    default: throw std::runtime_error("unexpected token in expression");
    }
}

inline Expr Parser::ParseFactor() {
    Expr left = ParseBase();
    if (peek().type == TokenType::Caret) {
        consume();
        Expr right = ParseFactor();
        left = BinOp{TokenType::Caret, std::make_unique<Expr>(std::move(left)), std::make_unique<Expr>(std::move(right))};
    }
    return left;
}

inline Expr Parser::ParseTerm() {
    Expr left = ParseFactor();
    while (peek().type == TokenType::Star || peek().type == TokenType::Slash) {
        TokenType op = consume().type;
        Expr right = ParseFactor();
        left = BinOp{op, std::make_unique<Expr>(std::move(left)), std::make_unique<Expr>(std::move(right))};
    }
    return left;
}

inline Expr Parser::ParseExpr() {
    Expr left = ParseTerm();
    while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
        TokenType op = consume().type;
        Expr right = ParseTerm();
        left = BinOp{op, std::make_unique<Expr>(std::move(left)), std::make_unique<Expr>(std::move(right))};
    }
    return left;
}
