#include "parser.hpp"

StmtPtr desugar_for(StmtPtr initializer, ExprPtr condition, ExprPtr step, StmtPtr body) {
    if (step) {
        std::vector<StmtPtr> step_block;
        step_block.push_back(std::move(body));
        step_block.push_back(make_stmt<ExprStmt>(std::move(step)));
        body = make_stmt<BlockStmt>(std::move(step_block));
    }

    if (!condition) {
        condition = make_expr<LiteralExpr>(true);
    }

    body = make_stmt<WhileStmt>(std::move(condition), std::move(body));

    if (initializer) {
        std::vector<StmtPtr> full_block;
        full_block.push_back(std::move(initializer));
        full_block.push_back(std::move(body));
        body = make_stmt<BlockStmt>(std::move(full_block));
    }

    return body;
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;

    while (!at_end()) {
        try {
            statements.push_back(declaration());
        } catch (...) {
            synchronize();
        }
    }

    return statements;
}

StmtPtr Parser::declaration() {
    if (match(TokenType::Let)) return var_declaration();
    if (match(TokenType::Function)) return func_declaration();
    return statement();
}

StmtPtr Parser::var_declaration() {
    Token name = consume(TokenType::Identifier, "Expect variable name.");

    ExprPtr initializer = nullptr;
    if (match(TokenType::Assign)) {
        initializer = expression();
    }
    
    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return make_stmt<LetStmt>(name, std::move(initializer));
}

StmtPtr Parser::func_declaration() {
    Token name = consume(TokenType::Identifier, "Expect function name.");
    
    consume(TokenType::LeftParen, "Expect '(' after function name.");

    std::vector<Token> params;

    if (!check(TokenType::RightParen)) {
        do {
            if (params.size() >= 255) {
                std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value
                    << "': Can't have more than 255 parameters.\n";
            }

            params.push_back(consume(TokenType::Identifier, "Expect parameter name."));
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightParen, "Expect ')' after parameters.");
    consume(TokenType::LeftCurly, "Expect '{' before function body.");

    StmtPtr body = block();

    return make_stmt<FunctionStmt>(name, std::move(params), std::move(body));
}

StmtPtr Parser::statement() {
    if (match(TokenType::Disp))      return disp_statement();
    if (match(TokenType::LeftCurly)) return block();
    if (match(TokenType::If))        return if_statement();
    if (match(TokenType::While))     return while_statement();
    if (match(TokenType::For))       return for_statement();
    if (match(TokenType::Return))    return return_statement();
    return expr_statement();
}

StmtPtr Parser::disp_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<DispStmt>(std::move(expr));
}

StmtPtr Parser::block() {
    std::vector<StmtPtr> statements;

    while (!check(TokenType::RightCurly) && !at_end()) {
        statements.push_back(declaration());
    }

    consume(TokenType::RightCurly, "Expect '}' after block.");
    return make_stmt<BlockStmt>(std::move(statements));
}

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

StmtPtr Parser::while_statement() {
    ExprPtr condition = expression();
    consume(TokenType::LeftCurly, "Expect '{' after expresion.");
    StmtPtr body = block();

    return make_stmt<WhileStmt>(std::move(condition), std::move(body));
}

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

StmtPtr Parser::return_statement() {
    ExprPtr expr = nullptr;
    if (!check(TokenType::Semicolon)) {
        expr = expression();
    }

    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<ReturnStmt>(std::move(expr));
}

StmtPtr Parser::expr_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return make_stmt<ExprStmt>(std::move(expr));
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = ternary();

    if (match(TokenType::Assign, TokenType::PlusEqual, TokenType::MinusEqual,
              TokenType::MultEqual, TokenType::DivEqual, TokenType::ModEqual))
    {
        Token op = previous();
        ExprPtr val = assignment();
        
        if (std::holds_alternative<VariableExpr>(*expr) || std::holds_alternative<IndexExpr>(*expr)) {
            return make_expr<AssignExpr>(std::move(expr), std::move(val), op);
        }

        std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value 
                  << "': Invalid assignment target.\n";
    }

    return expr;
}

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

ExprPtr Parser::logic_or() {
    ExprPtr expr = logic_and();
    
    while (match(TokenType::Or)) {
        Token op = previous();
        ExprPtr right = logic_and();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::logic_and() {
    ExprPtr expr = bit_or();
    
    while (match(TokenType::And)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::bit_or() {
    ExprPtr expr = bit_xor();
    
    while (match(TokenType::BitOr)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::bit_xor() {
    ExprPtr expr = bit_and();
    
    while (match(TokenType::BitXor)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::bit_and() {
    ExprPtr expr = equality();
    
    while (match(TokenType::BitAnd)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = make_expr<LogicalExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    
    while (match(TokenType::Equal, TokenType::NotEqual)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = bit_shift();

    while (match(TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual)) {
        Token op = previous();
        ExprPtr right = term();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::bit_shift() {
    ExprPtr expr = term();
    
    while (match(TokenType::BitShiftLeft, TokenType::BitShiftRight)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    
    while (match(TokenType::Plus, TokenType::Minus)) {
        Token op = previous();
        ExprPtr right = factor();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    
    while (match(TokenType::Mult, TokenType::Div, TokenType::Mod)) {
        Token op = previous();
        ExprPtr right = unary();
        expr = make_expr<BinaryExpr>(std::move(expr), std::move(right), op);
    }

    return expr;
}

ExprPtr Parser::unary() {
    if (match(TokenType::Not, TokenType::Minus, TokenType::BitNot)) {
        Token op = previous();
        ExprPtr right = unary();
        return make_expr<UnaryExpr>(std::move(right), op);
    }

    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();

    while (true) {
        if (match(TokenType::LeftParen)) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RightParen)) {
                do {
                    if (args.size() >= 255) {
                        std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value
                                  << "': Can't have more than 255 arguments.\n";
                    }
    
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
    
            consume(TokenType::RightParen, "Expect ')' after arguments.");
            expr = make_expr<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::LeftBracket)) {
            ExprPtr index = expression();
            consume(TokenType::RightBracket, "Expect ']' after index.");
            expr = make_expr<IndexExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::primary() {
    if (match(TokenType::LeftBracket)) return array_literal();
    if (match(TokenType::LeftCurly)) return object_literal();

    if (match(TokenType::Null))  return make_expr<LiteralExpr>(Value());
    if (match(TokenType::True))  return make_expr<LiteralExpr>(true);
    if (match(TokenType::False)) return make_expr<LiteralExpr>(false);

    if (match(TokenType::Integer)) return make_expr<LiteralExpr>(std::stoi(std::string(previous().Value)));
    if (match(TokenType::Float))   return make_expr<LiteralExpr>(std::stod(std::string(previous().Value)));
    if (match(TokenType::String))  return make_expr<LiteralExpr>(std::string(previous().Value));

    if (match(TokenType::Identifier)) return make_expr<VariableExpr>(previous());

    if (match(TokenType::LeftParen)) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression");
        return make_expr<GroupingExpr>(std::move(expr));
    }

    std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': Expect expresion\n";
    return nullptr;
}

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

ExprPtr Parser::object_literal() {
    std::unordered_map<std::string, ExprPtr> items;

    if (!check(TokenType::RightCurly)) {
        do {
            std::string key;
            if (match(TokenType::String, TokenType::Identifier)) {
                key = std::string(previous().Value);
            } else {
                std::cerr << "[Parse Error] Line " << peek().Line 
                          << " at '" << peek().Value 
                          << "': Expect string or identifier as object key.\n";
                return nullptr;
            }

            consume(TokenType::Colon, "Expect ':' after key in object literal.");

            ExprPtr value = expression();
            items[key] = std::move(value);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightCurly, "Expect '}' after object items.");
    return make_expr<ObjectExpr>(std::move(items));
}

void Parser::synchronize() {
    advance();
    while (!at_end()) {
        if (previous().Type == TokenType::Semicolon) return;

        switch (peek().Type) {
            case TokenType::Let:
            case TokenType::Function:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                break;
        }

        advance();
    }
}

template <typename... Args>
bool Parser::match(Args... args) {
    std::initializer_list<TokenType> types{args...};

    for (const TokenType &type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

Token Parser::consume(TokenType type, const std::string &msg) {
    if (check(type)) return advance();
    std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': " << msg << "\n";
}

bool Parser::check(TokenType type) {
    if (at_end()) return false;
    return peek().Type == type;
}

Token Parser::advance() {
    if (!at_end()) Curr++;
    return previous();
}

bool Parser::at_end() {
    return peek().Type == TokenType::Eof;
}

Token Parser::peek() {
    return Tokens[Curr];
}

Token Parser::previous() {
    return Tokens[Curr - 1];
}