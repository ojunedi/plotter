#pragma once

#include <cctype>
#include <ostream>
#include <string>
#include <vector>

enum class TokenType {
    Number,
    Ident,
    Plus,
    Minus,
    LParen,
    RParen,
    Slash,
    Star,
    Caret,
    End
};

struct Token {
    TokenType type;
    double value = 0; // For numbers
    std::string text = ""; // For identifers
};


inline std::ostream &operator<<(std::ostream &os, const Token &t) {
    switch (t.type) {
        case TokenType::Number: return os << "Token(Number, " << t.value << ")";
        case TokenType::Ident:  return os << "Token(Ident, \"" << t.text << "\")";
        case TokenType::Plus:   return os << "Token(Plus)";
        case TokenType::Minus:  return os << "Token(Minus)";
        case TokenType::Star:   return os << "Token(Star)";
        case TokenType::Slash:  return os << "Token(Slash)";
        case TokenType::Caret:  return os << "Token(Caret)";
        case TokenType::LParen: return os << "Token(LParen)";
        case TokenType::RParen: return os << "Token(RParen)";
        case TokenType::End:    return os << "Token(End)";
    }
    return os;
}

inline std::vector<Token> lex(const std::string &src) {

    std::vector<Token> tokens;
    int n = src.length();

    int i = 0;
    while (i < n) {

        while (i < n && std::isspace(src[i])) {i++;}

        if (isdigit(src[i]) || src[i] == '.') {
            std::string num;
            while (i < n && (isdigit(src[i]) || src[i] == '.'))
                num += src[i++];
            tokens.push_back(Token {TokenType::Number, std::stof(num), ""});
            continue;
        }
        if (std::isalpha(src[i])) {
            std::string ident;
            while (i < n && std::isalpha(src[i]))
                ident += src[i++];
            tokens.push_back(Token {TokenType::Ident, 0, ident});
            continue;
        }

        switch (src[i]) {
            case '+': tokens.push_back({TokenType::Plus}); break;
            case '-': tokens.push_back({TokenType::Minus}); break;
            case '*': tokens.push_back({TokenType::Star}); break;
            case '^': tokens.push_back({TokenType::Caret}); break;
            case '/': tokens.push_back({TokenType::Slash}); break;
            case '(': tokens.push_back({TokenType::LParen}); break;
            case ')': tokens.push_back({TokenType::RParen}); break;
            default: throw std::runtime_error(std::string("unexpected character: ") + src[i]);
        }
        i++;
    }
    tokens.push_back({TokenType::End});
    return tokens;
}
