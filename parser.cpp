#include "ast.hpp"
#include "lexer.hpp"
#include <stdexcept>


struct Parser {
    std::vector<Token> tokens;
    int pos = 0;

    Token peek() { return tokens[pos]; }
    Token consume() { return tokens[pos++]; }
    Token expect(TokenType t) {
        if (peek().type != t) throw std::runtime_error("unexpected token");
        return consume();
    }

};
