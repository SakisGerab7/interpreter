#include "parser.hpp"

StmtPtr desugar_for(StmtPtr initializer, ExprPtr condition, ExprPtr step, StmtPtr body) {
    if (step) {
        auto step_block = std::make_shared<std::vector<StmtPtr>>();
        step_block->push_back(std::move(body));
        step_block->push_back(make_stmt<ExprStmt>(std::move(step)));
        body = make_stmt<BlockStmt>(step_block);
    }

    if (!condition) {
        condition = make_expr<LiteralExpr>(true);
    }

    body = make_stmt<WhileStmt>(std::move(condition), std::move(body));

    if (initializer) {
        auto full_block = std::make_shared<std::vector<StmtPtr>>();
        full_block->push_back(std::move(initializer));
        full_block->push_back(std::move(body));
        body = make_stmt<BlockStmt>(full_block);
    }

    return body;
}

std::shared_ptr<std::vector<StmtPtr>> Parser::parse() {
    auto statements = std::make_shared<std::vector<StmtPtr>>();

    while (!at_end()) {
        try {
            statements->push_back(declaration());
        } catch (const LexerError &e) {
            std::cerr << e.what() << "\n";
        } catch (const ParseError &e) {
            std::cerr << e.what() << "\n";
            synchronize();
        }
    }

    return statements;
}

// declaration → var_declaration | func_declaration | struct_declaration | statement ;
StmtPtr Parser::declaration() {
    if (match(TokenType::Let)) return var_declaration();
    if (match(TokenType::Function)) return func_declaration();
    if (match(TokenType::Struct)) return struct_declaration();
    return statement();
}

// var_declaration → "let" IDENTIFIER ( "=" expression )? ("," IDENTIFIER ( "=" expression )? )* ";" ;
StmtPtr Parser::var_declaration() {
    // TODO: support multiple variable declarations in one statement
    auto name = consume(TokenType::Identifier, "Expect variable name.");

    ExprPtr initializer = nullptr;
    if (match(TokenType::Assign)) {
        initializer = expression();
    }
    
    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return make_stmt<LetStmt>(name, std::move(initializer));
}

// parameters → IDENTIFIER ( "," IDENTIFIER )* ;
std::vector<Token> Parser::parameters() {
    std::vector<Token> params;

    if (!check(TokenType::RightParen)) {
        do {
            if (params.size() >= 255) {
                throw ParseError(peek(), "Can't have more than 255 parameters.");
            }

            params.push_back(consume(TokenType::Identifier, "Expect parameter name."));
        } while (match(TokenType::Comma));
    }

    return params;
}

// func_declaration → "fn" IDENTIFIER "(" parameters? ")" "{" block_statements "}" ;
StmtPtr Parser::func_declaration() {
    auto name = consume(TokenType::Identifier, "Expect function name.");
    consume(TokenType::LeftParen, "Expect '(' after function name.");

    std::vector<Token> params = parameters();

    consume(TokenType::RightParen, "Expect ')' after parameters.");
    consume(TokenType::LeftCurly, "Expect '{' before function body.");

    auto body = block_statements();

    return make_stmt<FunctionStmt>(name, std::move(params), body);
}

// struct_declaration → "struct" IDENTIFIER "{" func_declaration* "}" ;
StmtPtr Parser::struct_declaration() {
    auto name = consume(TokenType::Identifier, "Expect struct name.");
    consume(TokenType::LeftCurly, "Expect '{' before struct body.");

    std::vector<StmtPtr> methods;

    while (!check(TokenType::RightCurly) && !at_end()) {
        consume(TokenType::Function, "Expect 'fn' keyword before method declaration.");
        methods.push_back(func_declaration());
    }

    consume(TokenType::RightCurly, "Expect '}' after struct body.");
    return make_stmt<StructStmt>(name, std::move(methods));
}

// statement → disp_statement | block | if_statement | while_statement | for_statement | return_statement | expr_statement ;
StmtPtr Parser::statement() {
    if (match(TokenType::Disp))      return disp_statement();
    if (match(TokenType::LeftCurly)) return block();
    if (match(TokenType::If))        return if_statement();
    if (match(TokenType::While))     return while_statement();
    if (match(TokenType::For))       return for_statement();
    if (match(TokenType::Return))    return return_statement();
    return expr_statement();
}

// disp_statement → "disp" expression ";" ;
StmtPtr Parser::disp_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<DispStmt>(std::move(expr));
}

StmtPtr Parser::block() {
    return make_stmt<BlockStmt>(block_statements());
}

// if_statement → "if" expression "{" block_statements "}" ( "else" statement )? ;
StmtPtr Parser::if_statement() {
    ExprPtr condition = expression();

    consume(TokenType::LeftCurly, "Expect '{' after expresion.");
    StmtPtr then_branch = block();
    StmtPtr else_branch = nullptr;
    
    if (match(TokenType::Else)) {
        else_branch = statement();
    }

    return make_stmt<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

// while_statement → "while" expression "{" block_statements "}" ;
StmtPtr Parser::while_statement() {
    ExprPtr condition = expression();
    consume(TokenType::LeftCurly, "Expect '{' after expresion.");
    StmtPtr body = block();

    return make_stmt<WhileStmt>(std::move(condition), std::move(body));
}

// for_statement → "for" ( var_declaration | expr_statement | ";" ) expression? ";" expression? "{" block_statements "}" ;
StmtPtr Parser::for_statement() {
    StmtPtr initializer;
    if (match(TokenType::Semicolon)) {
        initializer = nullptr;
    } else if (match(TokenType::Let)) {
        initializer = var_declaration();
    } else {
        initializer = expr_statement();
    }
    
    ExprPtr condition = nullptr;
    if (!match(TokenType::Semicolon)) {
        condition = expression();
        consume(TokenType::Semicolon, "Expect ';' after expression.");
    }

    ExprPtr step = nullptr;
    if (!match(TokenType::LeftCurly)) {
        step = expression();
        consume(TokenType::LeftCurly, "Expect '{' after expression.");
    }

    StmtPtr body = block();

    return desugar_for(std::move(initializer), std::move(condition), std::move(step), std::move(body));
}

// TODO:
// foreach_statement → "for" IDENTIFIER ("," IDENTIFIER)* "in" expression "{" block_statements "}" ;

// return_statement → "return" expression? ";" ;
StmtPtr Parser::return_statement() {
    ExprPtr expr = nullptr;
    if (!check(TokenType::Semicolon)) {
        expr = expression();
    }

    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<ReturnStmt>(std::move(expr));
}

// expr_statement → expression ";" ;
StmtPtr Parser::expr_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<ExprStmt>(std::move(expr));
}

// block_statements → declaration* "}" ;
std::shared_ptr<std::vector<StmtPtr>> Parser::block_statements() {
    auto statements = std::make_shared<std::vector<StmtPtr>>();

    while (!check(TokenType::RightCurly) && !at_end()) {
        statements->push_back(declaration());
    }

    consume(TokenType::RightCurly, "Expect '}' after block.");
    return statements;
}

// expression → assignment ;
ExprPtr Parser::expression() {
    return assignment();
}

// assignment → (call "." IDENTIFIER | IDENTIFIER) ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" ) assignment
//            | ternary ;
ExprPtr Parser::assignment() {
    ExprPtr expr = ternary();

    if (match(TokenType::Assign, TokenType::PlusEqual, TokenType::MinusEqual,
              TokenType::MultEqual, TokenType::DivEqual, TokenType::ModEqual))
    {
        Token op = previous();
        ExprPtr val = assignment();

        if (auto var = std::get_if<VariableExpr>(&*expr)) {
            return make_expr<AssignExpr>(var->name, std::move(val), op);
        }

        if (auto idx = std::get_if<IndexExpr>(&*expr)) {
            return make_expr<SetIndexExpr>(std::move(idx->target),
                                           std::move(idx->index),
                                           std::move(val), op);
        }

        if (auto dot = std::get_if<DotExpr>(&*expr)) {
            return make_expr<SetDotExpr>(std::move(dot->target),
                                         dot->key,
                                         std::move(val), op);
        }

        throw ParseError(peek(), "Invalid assignment target.");
    }

    return expr;
}

// ternary → logic_or ( "?" expression ":" ternary )* ;
ExprPtr Parser::ternary() {
    ExprPtr expr = logic_or();

    while (match(TokenType::Questionmark)) {
        ExprPtr right = expression();
        consume(TokenType::Colon, "Expect ':' after '?' branch of ternary expression");
        ExprPtr left = ternary();
        expr = make_expr<TernaryExpr>(std::move(expr), std::move(right), std::move(left));
    }

    return expr;
}

// logic_or → logic_and ( "||" logic_and )* ;
ExprPtr Parser::logic_or() {
    ExprPtr expr = logic_and();
    
    while (match(TokenType::Or)) {
        Token op = previous();
        ExprPtr right = logic_and();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// logic_and → bit_or ( "&&" bit_or )* ;
ExprPtr Parser::logic_and() {
    ExprPtr expr = bit_or();
    
    while (match(TokenType::And)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// bit_or → bit_xor ( "|" bit_xor )* ;
ExprPtr Parser::bit_or() {
    ExprPtr expr = bit_xor();
    
    while (match(TokenType::BitOr)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// bit_xor → bit_and ( "^" bit_and )* ;
ExprPtr Parser::bit_xor() {
    ExprPtr expr = bit_and();
    
    while (match(TokenType::BitXor)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// bit_and → equality ( "&" equality )* ;
ExprPtr Parser::bit_and() {
    ExprPtr expr = equality();
    
    while (match(TokenType::BitAnd)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// equality → comparison ( ( "==" | "!=" ) comparison )* ;
ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    
    while (match(TokenType::Equal, TokenType::NotEqual)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// comparison → bit_shift ( ( ">" | ">=" | "<" | "<=" ) bit_shift )* ;
ExprPtr Parser::comparison() {
    ExprPtr expr = bit_shift();

    while (match(TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual)) {
        Token op = previous();
        ExprPtr right = term();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// bit_shift → term ( ( "<<" | ">>" ) term )* ;
ExprPtr Parser::bit_shift() {
    ExprPtr expr = term();
    
    while (match(TokenType::BitShiftLeft, TokenType::BitShiftRight)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// term → factor ( ( "+" | "-" ) factor )* ;
ExprPtr Parser::term() {
    ExprPtr expr = factor();
    
    while (match(TokenType::Plus, TokenType::Minus)) {
        Token op = previous();
        ExprPtr right = factor();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// factor → unary ( ( "*" | "/" | "%" ) unary )* ;
ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    
    while (match(TokenType::Mult, TokenType::Div, TokenType::Mod)) {
        Token op = previous();
        ExprPtr right = unary();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

// unary → ( "!" | "-" | "~" | "++" | "--" ) unary | call ;
ExprPtr Parser::unary() {
    if (match(TokenType::Not, TokenType::Minus, TokenType::BitNot, TokenType::Increment, TokenType::Decrement)) {
        Token op = previous();
        ExprPtr right = unary();
        return make_expr<UnaryExpr>(std::move(right), op);
    }

    return call();
}

// call → primary ( "(" arguments? ")" | "." IDENTIFIER | "[" expression "]" )* ;
ExprPtr Parser::call() {
    ExprPtr expr = primary();

    while (true) {
        if (match(TokenType::Increment, TokenType::Decrement)) {
            Token op = previous();
            expr = make_expr<PostfixExpr>(std::move(expr), op);
        } else if (match(TokenType::LeftParen)) {
            std::vector<ExprPtr> args;

            if (!check(TokenType::RightParen)) {
                do {
                    if (args.size() >= 255) {
                        throw ParseError(peek(), "Can't have more than 255 arguments.");
                    }
    
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
    
            consume(TokenType::RightParen, "Expect ')' after arguments.");
            expr = make_expr<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::LeftBracket)) {
            auto index = expression();
            consume(TokenType::RightBracket, "Expect ']' after index.");
            expr = make_expr<IndexExpr>(std::move(expr), std::move(index));
        } else if (match(TokenType::Dot)) {
            Token key = consume(TokenType::Identifier, "Expect property name after '.'.");
            expr = make_expr<DotExpr>(std::move(expr), key);
        } else {
            break;
        }
    }

    return expr;
}

// primary → "true" | "false" | "null" | INTEGER | FLOAT | STRING | IDENTIFIER
//         | "(" expression ")" | array_literal | object_literal | lambda_expression | "self"
ExprPtr Parser::primary() {
    if (match(TokenType::LeftBracket)) return array_literal();
    if (match(TokenType::LeftCurly)) return object_literal();

    if (match(TokenType::Function)) return lambda_expression();

    if (match(TokenType::Null))  return make_expr<LiteralExpr>(Value());
    if (match(TokenType::True))  return make_expr<LiteralExpr>(true);
    if (match(TokenType::False)) return make_expr<LiteralExpr>(false);

    if (match(TokenType::Integer)) return make_expr<LiteralExpr>(std::stoi(previous().value));
    if (match(TokenType::Float))   return make_expr<LiteralExpr>(std::stod(previous().value));
    if (match(TokenType::String))  return make_expr<LiteralExpr>(previous().value);

    if (match(TokenType::Identifier)) return make_expr<VariableExpr>(previous());
    if (match(TokenType::Self)) return make_expr<SelfExpr>(previous());

    if (match(TokenType::Spawn)) {
        ExprPtr expr = nullptr;
        if (!check(TokenType::LeftCurly)) {
            expr = expression();
        }

        consume(TokenType::LeftCurly, "Expect '{' after 'spawn'");
        auto stmts = block_statements();
        
        return make_expr<SpawnExpr>(std::move(expr), stmts);
    }

    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression");
        return make_expr<GroupingExpr>(std::move(expr));
    }

    throw ParseError(peek(), "Expect expression.");
}

// array_literal → "[" ( expression ( "," expression )* )? "]" ;
ExprPtr Parser::array_literal() {
    std::vector<ExprPtr> elements;

    if (!check(TokenType::RightBracket)) {
        do {
            elements.push_back(expression());
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightBracket, "Expect ']' after array elements.");
    return make_expr<ArrayExpr>(std::move(elements));
}

// object_literal → "{" ( ( STRING | IDENTIFIER ) ":" expression ( "," ( STRING | IDENTIFIER ) ":" expression )* )? "}" ;
ExprPtr Parser::object_literal() {
    std::unordered_map<std::string, ExprPtr> items;

    if (!check(TokenType::RightCurly)) {
        do {
            std::string key;
            if (match(TokenType::String, TokenType::Identifier)) {
                key = previous().value;
            } else {
                throw ParseError(peek(), "Expect string or identifier as object key.");
            }

            consume(TokenType::Colon, "Expect ':' after key in object literal.");

            auto value = expression();
            items[key] = std::move(value);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightCurly, "Expect '}' after object items.");
    return make_expr<ObjectExpr>(std::move(items));
}

// lambda_expression → "fn" "(" parameters? ")" ( "->" expression | "{" block_statements "}" ) ;
ExprPtr Parser::lambda_expression() {    
    consume(TokenType::LeftParen, "Expect '(' after 'fn' keyword.");
    
    std::vector<Token> params = parameters();

    consume(TokenType::RightParen, "Expect ')' after parameters.");
    
    if (match(TokenType::Arrow)) {
        auto return_expr = expression();
        
        auto body = std::make_shared<std::vector<StmtPtr>>();
        body->push_back(make_stmt<ReturnStmt>(std::move(return_expr)));

        return make_expr<LambdaExpr>(std::move(params), body);
    }

    consume(TokenType::LeftCurly, "Expect '{' before function body.");

    auto body = block_statements();

    return make_expr<LambdaExpr>(std::move(params), body);
}

void Parser::synchronize() {
    advance();
    while (!at_end()) {
        if (previous().type == TokenType::Semicolon) return;

        switch (peek().type) {
            case TokenType::Let:
            case TokenType::Function:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
            case TokenType::Struct:
                return;
            default:
                break;
        }

        advance();
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

template <typename... Args>
bool Parser::match(Args... args) {
    return ((match(args)) || ...);
}

inline Token Parser::consume(TokenType type, const std::string &msg) {
    if (check(type)) return advance();
    throw ParseError(peek(), msg);
}

inline bool Parser::check(TokenType type) {
    if (at_end()) return false;
    return curr.type == type;
}

inline Token Parser::advance() {
    if (!at_end()) {
        prev = curr;
        curr = lexer.next_token();
    }

    return prev;
}

inline bool Parser::at_end() {
    return curr.type == TokenType::Eof;
}

inline Token Parser::peek() {
    return curr;
}

inline Token Parser::previous() {
    return prev;
}