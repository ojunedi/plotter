#pragma once
#include "ast.hpp"
#include "lexer.hpp"
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
            if (prev.text == "x")   return Variable{};
            if (prev.text == "pi")  return Number{M_PI};
            if (prev.text == "e")   return Number{M_E};
            if (peek().type == TokenType::LParen) {
                consume();
                Expr arg = ParseExpr();
                expect(TokenType::RParen);
                return FuncCall{prev.text, std::make_unique<Expr>(std::move(arg))};
            }
            throw std::runtime_error("unknown identifier: " + prev.text);

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
    for (;;) {
        TokenType t = peek().type;
        TokenType op;
        if (t == TokenType::Star || t == TokenType::Slash) {
            op = consume().type;
        } else if (t == TokenType::Number || t == TokenType::Ident || t == TokenType::LParen) {
            op = TokenType::Star; // implicit multiplication, e.g. 2x, 2sin(x), (x+1)(x-2)
        } else {
            break;
        }
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
