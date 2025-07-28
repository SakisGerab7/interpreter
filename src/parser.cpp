#include "parser.hpp"

StmtPtr desugar_for(StmtPtr initializer, ExprPtr condition, ExprPtr step, StmtPtr body) {
    if (step) {
        std::vector<StmtPtr> stepBlock;
        stepBlock.push_back(std::move(body));
        stepBlock.push_back(std::make_unique<ExprStmt>(std::move(step)));
        body = std::make_unique<BlockStmt>(std::move(stepBlock));
    }

    if (!condition) {
        condition = std::make_unique<LiteralExpr>(true);
    }

    body = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

    if (initializer) {
        std::vector<StmtPtr> fullBlock;
        fullBlock.push_back(std::move(initializer));
        fullBlock.push_back(std::move(body));
        body = std::make_unique<BlockStmt>(std::move(fullBlock));
    }

    return body;
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;

    while (!at_end()) {
        statements.push_back(declaration());
    }

    return statements;
}

StmtPtr Parser::declaration() {
    if (match(TokenType::Let)) return var_declaration();
    return statement();
}

StmtPtr Parser::var_declaration() {
    Token name = consume(TokenType::Identifier, "Expect variable name.");

    ExprPtr initializer = nullptr;
    if (match(TokenType::Assign)) {
        initializer = expression();
    }
    
    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return std::make_unique<LetStmt>(name, std::move(initializer));
}

StmtPtr Parser::statement() {
    if (match(TokenType::Disp))      return disp_statement();
    if (match(TokenType::LeftCurly)) return block();
    if (match(TokenType::If))        return if_statement();
    if (match(TokenType::While))     return while_statement();
    if (match(TokenType::For))       return for_statement();
    return expr_statement();
}

StmtPtr Parser::disp_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<DispStmt>(std::move(expr));
}

StmtPtr Parser::block() {
    std::vector<StmtPtr> statements;

    while (!check(TokenType::RightCurly) && !at_end()) {
        statements.push_back(declaration());
    }

    consume(TokenType::RightCurly, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

StmtPtr Parser::if_statement() {
    ExprPtr condition = expression();

    consume(TokenType::LeftCurly, "Expect '{' after expresion.");
    StmtPtr then_branch = block();
    StmtPtr else_branch = nullptr;
    
    if (match(TokenType::Else)) {
        else_branch = statement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

StmtPtr Parser::while_statement() {
    ExprPtr condition = expression();
    consume(TokenType::LeftCurly, "Expect '{' after expresion.");
    StmtPtr body = block();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
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

StmtPtr Parser::expr_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = logic_or();

    if (match(TokenType::Assign)) {
        ExprPtr val = assignment();
        
        if (VariableExpr *var_expr = dynamic_cast<VariableExpr *>(expr.get())) {
            return std::make_unique<AssignExpr>(var_expr->Name, std::move(val));
        }
        
        std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': Invalid assignment target.\n";
    }

    return expr;
}

ExprPtr Parser::logic_or() {
    ExprPtr expr = logic_and();
    
    while (match(TokenType::Or)) {
        Token op = previous();
        ExprPtr right = logic_and();
        expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::logic_and() {
    ExprPtr expr = equality();
    
    while (match(TokenType::And)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    
    while (match(TokenType::Equal, TokenType::NotEqual)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();

    while (match(TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual)) {
        Token op = previous();
        ExprPtr right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    
    while (match(TokenType::Plus, TokenType::Minus)) {
        Token op = previous();
        ExprPtr right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    
    while (match(TokenType::Mult, TokenType::Div, TokenType::Mod)) {
        Token op = previous();
        ExprPtr right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

ExprPtr Parser::unary() {
    if (match(TokenType::Not, TokenType::Minus)) {
        Token op = previous();
        ExprPtr right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
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
    
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::LeftBracket)) {
            ExprPtr index = expression();

            consume(TokenType::RightBracket, "Expect ']' after index.");
            
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::primary() {
    if (match(TokenType::LeftBracket)) return array_literal();

    if (match(TokenType::Null)) return std::make_unique<LiteralExpr>(Value());
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);

    if (match(TokenType::Integer)) return std::make_unique<LiteralExpr>(std::stoi(previous().Value));
    if (match(TokenType::Float)) return std::make_unique<LiteralExpr>(std::stod(previous().Value));
    if (match(TokenType::String)) return std::make_unique<LiteralExpr>(previous().Value);

    if (match(TokenType::Identifier)) return std::make_unique<VariableExpr>(previous());

    if (match(TokenType::LeftParen)) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': Expect expresion\n";
}

ExprPtr Parser::array_literal() {
    std::vector<ExprPtr> elements;

    if (!check(TokenType::RightBracket)) {
        do {
            elements.push_back(expression());
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RightBracket, "Expect ']' after array elements.");
    return std::make_unique<ArrayExpr>(std::move(elements));
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
    if (!at_end()) current++;
    return previous();
}

bool Parser::at_end() {
    return peek().Type == TokenType::Eof;
}

Token Parser::peek() {
    return tokens.at(current);
}

Token Parser::previous() {
    return tokens.at(current - 1);
}