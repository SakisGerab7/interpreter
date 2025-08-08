#include "lexer.hpp"

void Lexer::tokenize() {
    while (!at_end()) {
        Start = Curr;
        next_token();
    }

    Tokens.emplace_back(TokenType::Eof, "<EOF>", Line);
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
        
        case ',': add_token(TokenType::Comma);        break;
        case '.': add_token(TokenType::Dot);          break;
        case ';': add_token(TokenType::Semicolon);    break;
        case '?': add_token(TokenType::Questionmark); break;
        case ':': add_token(TokenType::Colon);        break;

        case '~': add_token(TokenType::BitNot);       break;
        case '^': add_token(TokenType::BitXor);       break;
        
        case '+': add_token(match('=') ? TokenType::PlusEqual    : TokenType::Plus);    break;
        case '-': add_token(match('=') ? TokenType::MinusEqual   : TokenType::Minus);   break;
        case '*': add_token(match('=') ? TokenType::MultEqual    : TokenType::Mult);    break; 
        case '%': add_token(match('=') ? TokenType::ModEqual     : TokenType::Mod);     break;
        case '!': add_token(match('=') ? TokenType::NotEqual     : TokenType::Not);     break;
        case '>': add_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '<': add_token(match('=') ? TokenType::LessEqual    : TokenType::Less);    break;
        case '=': add_token(match('=') ? TokenType::Equal        : TokenType::Assign);  break;
        
        case '&': add_token(match('&') ? TokenType::And : TokenType::BitAnd); break;
        case '|': add_token(match('|') ? TokenType::Or  : TokenType::BitOr);  break;

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
        case '\n': Line++; break;
        
        default: {
            if (isdigit(c)) {
                next_number();
            } else if (isalpha(c) || c == '_') {
                next_identifier();
            } else {
                std::cerr << "Line " << Line << ": Unexpected character '" << c << "'\n";
            }
        } break;
    }
}

void Lexer::skip_multiline_comment() {
    while (!(peek() == '*' && peek_next() == '/') && !at_end()) {
        if (peek() == '\n') Line++;
        advance();
    }

    if (at_end()) {
        std::cerr << "Line " << Line << ": Unterminated multiline comment\n";
        return;
    }

    advance();
    advance();
}

void Lexer::next_identifier() {
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
    };

    std::string_view value = Src.substr(Start, Curr - Start);
    
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
        if (peek() == '\n') Line++;
        advance();
    }

    if (at_end()) {
        std::cerr << "Line " << Line << ": Unterminated string\n";
        return;
    }

    advance();
    add_token(TokenType::String, Src.substr(Start + 1, Curr - Start - 2));
}

bool Lexer::match(char c) {
    if (at_end() || Src.at(Curr) != c) return false;
    Curr++;
    return true;
}

void Lexer::add_token(TokenType type) {
    add_token(type, Src.substr(Start, Curr - Start));
}

void Lexer::add_token(TokenType type, std::string_view value) {
    Tokens.emplace_back(type, value, Line);
}

char Lexer::peek() {
    return !at_end() ? Src[Curr] : '\0';
}

char Lexer::peek_next() {
    return (Curr + 1 < Src.size()) ? Src[Curr + 1] : '\0';
}

char Lexer::advance() {
    return Src[Curr++];
}

bool Lexer::at_end() {
    return Curr >= Src.size();
}