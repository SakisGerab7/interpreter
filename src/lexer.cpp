#include "lexer.hpp"

std::vector<Token> Lexer::tokenize() {
    while (!at_end()) {
        start = current;
        next_token();
    }

    tokens.emplace_back(TokenType::Eof, "<EOF>", line);
    return tokens;
}

void Lexer::next_token() {
    char c = advance();

    switch (c) {
        case '(': add_token(TokenType::LeftParen);    break;
        case ')': add_token(TokenType::RightParen);   break;
        case '[': add_token(TokenType::LeftBracket);  break;
        case ']': add_token(TokenType::RightBracket); break;
        case '{': add_token(TokenType::LeftCurly);    break;
        case '}': add_token(TokenType::RightCurly);   break;
        
        case ',': add_token(TokenType::Comma);     break;
        case '.': add_token(TokenType::Dot);       break;
        case ';': add_token(TokenType::Semicolon); break;
        
        case '+': add_token(match('=') ? TokenType::PlusEqual    : TokenType::Plus);    break;
        case '-': add_token(match('=') ? TokenType::MinusEqual   : TokenType::Minus);   break;
        case '*': add_token(match('=') ? TokenType::MultEqual    : TokenType::Mult);    break; 
        case '%': add_token(match('=') ? TokenType::ModEqual     : TokenType::Mod);     break;
        case '!': add_token(match('=') ? TokenType::NotEqual     : TokenType::Not);     break;
        case '>': add_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '<': add_token(match('=') ? TokenType::LessEqual    : TokenType::Less);    break;
        case '=': add_token(match('=') ? TokenType::Equal        : TokenType::Assign);  break;
        
        case '&': if (match('&')) add_token(TokenType::And); break;
        case '|': if (match('|')) add_token(TokenType::Or);  break;

        case '/': {
            if (match('/')) {
                while (peek() != '\n' && !at_end()) advance();
            } else if (match('*')) {
                skip_multiline_comment();
            } else {
                add_token(match('=') ? TokenType::DivEqual : TokenType::Div);
            }
        } break;
            
        case '"': next_string(); break;

        case ' ': case '\r': case '\t': break;
        case '\n': line++; break;
        
        default: {
            if (isdigit(c)) {
                next_number();
            } else if (isalpha(c) || c == '_') {
                next_identifier();
            } else {
                std::cerr << "Line " << line << ": Unexpected character '" << c << "'\n";
            }
        } break;
    }
}

void Lexer::skip_multiline_comment() {
    while (!(peek() == '*' && peek_next() == '/') && !at_end()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (at_end()) {
        std::cerr << "Line " << line << ": Unterminated multiline comment\n";
        return;
    }

    advance();
    advance();
}

void Lexer::next_identifier() {
    while (isalnum(peek()) || peek() == '_') advance();

    static std::unordered_map<std::string, TokenType> keywords = {
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
    };

    std::string value = source.substr(start, current - start);
    
    try {
        add_token(keywords.at(value));
    } catch (std::out_of_range) {
        add_token(TokenType::Identifier);
    }
}

void Lexer::next_number() {
    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
        add_token(TokenType::Float);
    } else {
        add_token(TokenType::Integer);
    }
}

void Lexer::next_string() {
    while (peek() != '"' && !at_end()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (at_end()) {
        std::cerr << "Line " << line << ": Unterminated string\n";
        return;
    }

    advance();
    add_token(TokenType::String, source.substr(start + 1, current - start - 2));
}

bool Lexer::match(char c) {
    if (at_end() || source[current] != c) return false;
    current++;
    return true;
}

void Lexer::add_token(TokenType type) {
    add_token(type, source.substr(start, current - start));
}

void Lexer::add_token(TokenType type, const std::string& value) {
    tokens.emplace_back(type, value, line);
}

char Lexer::peek() {
    return !at_end() ? source[current] : '\0';
}

char Lexer::peek_next() {
    return (current + 1 < source.size()) ? source[current + 1] : '\0';
}

char Lexer::advance() {
    return source[current++];
}

bool Lexer::at_end() {
    return current >= source.size();
}