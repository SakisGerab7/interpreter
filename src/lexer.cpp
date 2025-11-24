#include "lexer.hpp"

Token Lexer::next_token() {
    while (!at_end()) {
        start = curr;
        char c = advance();

        switch (c) {
            case '\0': break;
            case '(': return token_from(TokenType::LeftParen);    break;
            case ')': return token_from(TokenType::RightParen);   break;
            case '[': return token_from(TokenType::LeftBracket);  break;
            case ']': return token_from(TokenType::RightBracket); break;
            case '{': return token_from(TokenType::LeftCurly);    break;
            case '}': return token_from(TokenType::RightCurly);   break;
            
            case ',': return token_from(TokenType::Comma);        break;
            case '.': return token_from(TokenType::Dot);          break;
            case ';': return token_from(TokenType::Semicolon);    break;
            case '?': return token_from(TokenType::Questionmark); break;
            case ':': return token_from(TokenType::Colon);        break;

            case '~': return token_from(TokenType::BitNot);       break;
            case '^': return token_from(TokenType::BitXor);       break;
            
            case '+': return token_from(match('=') ? TokenType::PlusEqual :
                                        match('+') ? TokenType::Increment :
                                        TokenType::Plus); break;
            case '-': return token_from(match('=') ? TokenType::MinusEqual :
                                        match('-') ? TokenType::Decrement :
                                        match('>') ? TokenType::Arrow :
                                        TokenType::Minus); break;

            case '*': return token_from(match('=') ? TokenType::MultEqual    : TokenType::Mult);    break; 
            case '%': return token_from(match('=') ? TokenType::ModEqual     : TokenType::Mod);     break;
            case '!': return token_from(match('=') ? TokenType::NotEqual     : TokenType::Not);     break;
            case '>': return token_from(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
            case '<': return token_from(match('=') ? TokenType::LessEqual    : TokenType::Less);    break;
            case '=': return token_from(match('=') ? TokenType::Equal        : TokenType::Assign);  break;
            
            case '&': return token_from(match('&') ? TokenType::And : TokenType::BitAnd); break;
            case '|': return token_from(match('|') ? TokenType::Or  : TokenType::BitOr);  break;

            case '/': {
                if (match('/')) {
                    while (peek() != '\n' && !at_end()) advance();
                } else if (match('*')) {
                    skip_multiline_comment();
                } else {
                    return token_from(match('=') ? TokenType::DivEqual : TokenType::Div);
                }
            } break;
                
            case '"': return next_string(); break;

            case ' ': case '\r': case '\t': break;
            case '\n': line++; break;
            
            default: {
                if (isdigit(c)) {
                    return next_number();
                } else if (isalpha(c) || c == '_') {
                    return next_identifier();
                } else {
                    throw LexerError(line, std::string("Unexpected character '") + c + "'");
                }
            } break;
        }
    }

    return token_from(TokenType::Eof, "<EOF>");
}

void Lexer::skip_multiline_comment() {
    while (!(peek() == '*' && peek_next() == '/') && !at_end()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (at_end()) {
        throw LexerError(line, "Unterminated multiline comment");
    }

    advance();
    advance();
}

Token Lexer::next_identifier() {
    while (isalnum(peek()) || peek() == '_') advance();

    static std::unordered_map<std::string_view, TokenType> keywords = {
        { "let",    TokenType::Let      },
        { "struct", TokenType::Struct   },
        { "fn",     TokenType::Function },
        { "true",   TokenType::True     },
        { "false",  TokenType::False    },
        { "for",    TokenType::For      },
        { "in",     TokenType::In       },
        { "while",  TokenType::While    },
        { "if",     TokenType::If       },
        { "else",   TokenType::Else     },
        { "null",   TokenType::Null     },
        { "return", TokenType::Return   },
        { "self",   TokenType::Self     },
        { "disp",   TokenType::Disp     },
        { "spawn",  TokenType::Spawn    },
    };

    if (auto it = keywords.find(src.substr(start, curr - start)); it != keywords.end()) {
        return token_from(it->second);
    }

    return token_from(TokenType::Identifier);
}

Token Lexer::next_number() {
    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
        return token_from(TokenType::Float);
    } else {
        return token_from(TokenType::Integer);
    }
}

Token Lexer::next_string() {
    while (peek() != '"' && !at_end()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (at_end()) {
        throw LexerError(line, "Unterminated string literal");
    }

    advance();
    return token_from(TokenType::String, src.substr(start + 1, curr - start - 2));
}

bool Lexer::match(char c) {
    if (at_end() || peek() != c) return false;
    advance();
    return true;
}

Token Lexer::token_from(TokenType type) {
    return Token(type, src.substr(start, curr - start), line);
}

Token Lexer::token_from(TokenType type, std::string_view value) {
    return Token(type, value, line);
}

char Lexer::peek() {
    return !at_end() ? src[curr] : '\0';
}

char Lexer::peek_next() {
    return (curr + 1 < src.size()) ? src[curr + 1] : '\0';
}

char Lexer::advance() {
    return src[curr++];
}

bool Lexer::at_end() {
    return curr >= src.size();
}