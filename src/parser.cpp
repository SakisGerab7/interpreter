#include "parser.hpp"

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
    if (match(TokenType::Disp)) return disp_statement();
    if (match(TokenType::LeftCurly)) return block();
    if (match(TokenType::If)) return if_statement();
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

StmtPtr Parser::expr_statement() {
    ExprPtr expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = equality();

    if (match(TokenType::Assign)) {
        ExprPtr val = assignment();
        
        VariableExpr *var_expr = dynamic_cast<VariableExpr *>(expr.get());
        if (var_expr) {
            return std::make_unique<AssignExpr>(var_expr->Name, std::move(val));
        }
        
        std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': Invalid assignment target.\n";
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

    return primary();
}

ExprPtr Parser::primary() {
    if (match(TokenType::Null)) return std::make_unique<LiteralExpr>(Value());
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);

    if (match(TokenType::Integer)) return std::make_unique<LiteralExpr>(std::stoi(previous().Value));
    if (match(TokenType::Float)) return std::make_unique<LiteralExpr>(std::stof(previous().Value));
    if (match(TokenType::String)) return std::make_unique<LiteralExpr>(previous().Value);

    if (match(TokenType::Identifier)) return std::make_unique<VariableExpr>(previous());

    if (match(TokenType::LeftParen)) {
        ExprPtr expr = expression();
        consume(TokenType::RightParen, "Expect ')' after expression");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    std::cerr << "[Parse Error] Line " << peek().Line << " at '" << peek().Value << "': Expect expresion\n";
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