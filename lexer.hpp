#pragma once

#include <cctype>
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


inline std::vector<Token> lex(const std::string &src) {

    std::vector<Token> tokens;
    int n = src.length();

    int i = 0;
    while (i < n) {

        if (std::isspace(src[i])) {i++; continue;}

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
