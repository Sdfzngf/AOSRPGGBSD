module;

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

export module Engine.Basics.Lua.ASTGen;

namespace {

// ---------- 1. Token 定义 ----------
enum class TokenType : uint8_t {
    IDENTIFIER,
    NUMBER,
    STRING,
    KEYWORD_FUNCTION,
    KEYWORD_END,
    KEYWORD_IF,
    KEYWORD_THEN,
    KEYWORD_WHILE,
    KEYWORD_DO,
    KEYWORD_RETURN,
    KEYWORD_AND,
    KEYWORD_OR,
    KEYWORD_NOT,
    KEYWORD_LOCAL,
    KEYWORD_FOR,
    KEYWORD_IN,
    KEYWORD_BREAK,
    KEYWORD_NIL,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    SYMBOL_LPAREN,
    SYMBOL_RPAREN,
    SYMBOL_LBRACK,
    SYMBOL_RBRACK,
    SYMBOL_LCURLY,
    SYMBOL_RCURLY,
    SYMBOL_COMMA,
    SYMBOL_DOT,
    SYMBOL_COLON,
    SYMBOL_SEMICOLON,
    SYMBOL_PLUS,
    SYMBOL_MINUS,
    SYMBOL_STAR,
    SYMBOL_SLASH,
    SYMBOL_PERCENT,
    SYMBOL_CARET,
    SYMBOL_TILDE,
    SYMBOL_EQ,
    SYMBOL_NE,
    SYMBOL_LT,
    SYMBOL_GT,
    SYMBOL_LE,
    SYMBOL_GE,
    SYMBOL_CONCAT,
    SYMBOL_ASSIGN,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;
};

// ---------- 2. 词法分析器 ----------
class Lexer {
public:
    Lexer(std::string src)
        : source(std::move(src))
    {
    }

    auto tokenize() -> std::vector<Token>
    {
        std::vector<Token> tokens;
        while (true) {
            skipWhitespaceAndComments();
            if (pos >= source.size()) {
                tokens.push_back(Token { .type = TokenType::END_OF_FILE, .text = "EOF", .line = line, .column = col });
                break;
            }
            char c = source.at(pos);
            if (isalpha(c) || c == '_') {
                tokens.push_back(readIdentifier());
            } else if (isdigit(c) || (c == '.' && pos + 1 < source.size() && isdigit(source.at(pos + 1)))) {
                tokens.push_back(readNumber());
            } else if (c == '"' || c == '\'') {
                tokens.push_back(readString());
            } else {
                tokens.push_back(readSymbol());
            }
        }
        return tokens;
    }

private:
    std::string source;
    size_t pos { 0 };
    int line { 1 }, col { 1 };

    void skipWhitespaceAndComments()
    {
        while (pos < source.size()) {
            if (source.at(pos) == ' ' || source.at(pos) == '\t' || source.at(pos) == '\r') {
                if (source.at(pos) == '\n') {
                    line++;
                    col = 1;
                } else
                    col++;
                pos++;
            } else if (source.at(pos) == '\n') {
                line++;
                col = 1;
                pos++;
            } else if (source.at(pos) == '-' && pos + 1 < source.size() && source.at(pos + 1) == '-') {
                pos += 2;
                while (pos < source.size() && source.at(pos) != '\n')
                    pos++;
            } else if (source.at(pos) == '-' && pos + 1 < source.size() && source.at(pos + 1) == '[' && pos + 2 < source.size() && source.at(pos + 2) == '[') {
                pos += 3;
                while (pos < source.size() && !(source.at(pos) == ']' && pos + 1 < source.size() && source.at(pos + 1) == ']'))
                    pos++;
                pos += 2;
            } else {
                break;
            }
        }
    }

    auto readIdentifier() -> Token
    {
        int startCol = col;
        std::string text;
        while (pos < source.size() && (isalnum(source.at(pos)) || source.at(pos) == '_')) {
            text += source.at(pos);
            pos++;
            col++;
        }
        if (text == "function")
            return Token { .type = TokenType::KEYWORD_FUNCTION, .text = text, .line = line, .column = startCol };
        if (text == "end")
            return Token { .type = TokenType::KEYWORD_END, .text = text, .line = line, .column = startCol };
        if (text == "if")
            return Token { .type = TokenType::KEYWORD_IF, .text = text, .line = line, .column = startCol };
        if (text == "then")
            return Token { .type = TokenType::KEYWORD_THEN, .text = text, .line = line, .column = startCol };
        if (text == "while")
            return Token { .type = TokenType::KEYWORD_WHILE, .text = text, .line = line, .column = startCol };
        if (text == "do")
            return Token { .type = TokenType::KEYWORD_DO, .text = text, .line = line, .column = startCol };
        if (text == "return")
            return Token { .type = TokenType::KEYWORD_RETURN, .text = text, .line = line, .column = startCol };
        if (text == "and")
            return Token { .type = TokenType::KEYWORD_AND, .text = text, .line = line, .column = startCol };
        if (text == "or")
            return Token { .type = TokenType::KEYWORD_OR, .text = text, .line = line, .column = startCol };
        if (text == "not")
            return Token { .type = TokenType::KEYWORD_NOT, .text = text, .line = line, .column = startCol };
        if (text == "local")
            return Token { .type = TokenType::KEYWORD_LOCAL, .text = text, .line = line, .column = startCol };
        if (text == "for")
            return Token { .type = TokenType::KEYWORD_FOR, .text = text, .line = line, .column = startCol };
        if (text == "in")
            return Token { .type = TokenType::KEYWORD_IN, .text = text, .line = line, .column = startCol };
        if (text == "break")
            return Token { .type = TokenType::KEYWORD_BREAK, .text = text, .line = line, .column = startCol };
        if (text == "nil")
            return Token { .type = TokenType::KEYWORD_NIL, .text = text, .line = line, .column = startCol };
        if (text == "true")
            return Token { .type = TokenType::KEYWORD_TRUE, .text = text, .line = line, .column = startCol };
        if (text == "false")
            return Token { .type = TokenType::KEYWORD_FALSE, .text = text, .line = line, .column = startCol };
        return Token { .type = TokenType::IDENTIFIER, .text = text, .line = line, .column = startCol };
    }

    auto readNumber() -> Token
    {
        int startCol = col;
        std::string text;
        bool hasDot = false;
        while (pos < source.size() && (isdigit(source.at(pos)) || source.at(pos) == '.')) {
            if (source.at(pos) == '.') {
                if (hasDot)
                    break;
                hasDot = true;
            }
            text += source.at(pos);
            pos++;
            col++;
        }
        return { .type = TokenType::NUMBER, .text = text, .line = line, .column = startCol };
    }

    auto readString() -> Token
    {
        int startCol = col;
        char quote = source.at(pos);
        pos++;
        col++;
        std::string text;
        while (pos < source.size() && source.at(pos) != quote) {
            if (source.at(pos) == '\\' && pos + 1 < source.size()) {
                // 简单处理转义（仅支持常见）
                char esc = source.at(pos + 1);
                switch (esc) {
                case 'n':
                    text += '\n';
                    break;
                case 't':
                    text += '\t';
                    break;
                case 'r':
                    text += '\r';
                    break;
                default:
                    text += esc;
                    break;
                }
                pos += 2;
                col += 2;
            } else {
                text += source.at(pos);
                pos++;
                col++;
            }
        }
        if (pos < source.size()) {
            pos++;
            col++;
        }
        return { .type = TokenType::STRING, .text = text, .line = line, .column = startCol };
    }

    auto readSymbol() -> Token
    {
        char c = source.at(pos);
        int startCol = col;
        pos++;
        col++;
        if (pos < source.size()) {
            char next = source.at(pos);
            if (c == '=' && next == '=') {
                pos++;
                col++;
                return { .type = TokenType::SYMBOL_EQ, .text = "==", .line = line, .column = startCol };
            }
            if (c == '~' && next == '=') {
                pos++;
                col++;
                return { .type = TokenType::SYMBOL_NE, .text = "~=", .line = line, .column = startCol };
            }
            if (c == '<' && next == '=') {
                pos++;
                col++;
                return { .type = TokenType::SYMBOL_LE, .text = "<=", .line = line, .column = startCol };
            }
            if (c == '>' && next == '=') {
                pos++;
                col++;
                return { .type = TokenType::SYMBOL_GE, .text = ">=", .line = line, .column = startCol };
            }
            if (c == '.' && next == '.') {
                pos++;
                col++;
                if (pos < source.size() && source.at(pos) == '.') {
                    pos++;
                    col++;
                    return { .type = TokenType::SYMBOL_CONCAT, .text = "...", .line = line, .column = startCol };
                }
                return { .type = TokenType::SYMBOL_CONCAT, .text = "..", .line = line, .column = startCol };
            }
        }
        switch (c) {
        case '(':
            return { .type = TokenType::SYMBOL_LPAREN, .text = "(", .line = line, .column = startCol };
        case ')':
            return { .type = TokenType::SYMBOL_RPAREN, .text = ")", .line = line, .column = startCol };
        case '[':
            return { .type = TokenType::SYMBOL_LBRACK, .text = "[", .line = line, .column = startCol };
        case ']':
            return { .type = TokenType::SYMBOL_RBRACK, .text = "]", .line = line, .column = startCol };
        case '{':
            return { .type = TokenType::SYMBOL_LCURLY, .text = "{", .line = line, .column = startCol };
        case '}':
            return { .type = TokenType::SYMBOL_RCURLY, .text = "}", .line = line, .column = startCol };
        case ',':
            return { .type = TokenType::SYMBOL_COMMA, .text = ",", .line = line, .column = startCol };
        case '.':
            return { .type = TokenType::SYMBOL_DOT, .text = ".", .line = line, .column = startCol };
        case ':':
            return { .type = TokenType::SYMBOL_COLON, .text = ":", .line = line, .column = startCol };
        case ';':
            return { .type = TokenType::SYMBOL_SEMICOLON, .text = ";", .line = line, .column = startCol };
        case '+':
            return { .type = TokenType::SYMBOL_PLUS, .text = "+", .line = line, .column = startCol };
        case '-':
            return { .type = TokenType::SYMBOL_MINUS, .text = "-", .line = line, .column = startCol };
        case '*':
            return { .type = TokenType::SYMBOL_STAR, .text = "*", .line = line, .column = startCol };
        case '/':
            return { .type = TokenType::SYMBOL_SLASH, .text = "/", .line = line, .column = startCol };
        case '%':
            return { .type = TokenType::SYMBOL_PERCENT, .text = "%", .line = line, .column = startCol };
        case '^':
            return { .type = TokenType::SYMBOL_CARET, .text = "^", .line = line, .column = startCol };
        case '~':
            return { .type = TokenType::SYMBOL_TILDE, .text = "~", .line = line, .column = startCol };
        case '=':
            return { .type = TokenType::SYMBOL_ASSIGN, .text = "=", .line = line, .column = startCol };
        case '<':
            return { .type = TokenType::SYMBOL_LT, .text = "<", .line = line, .column = startCol };
        case '>':
            return { .type = TokenType::SYMBOL_GT, .text = ">", .line = line, .column = startCol };
        default:
            return { .type = TokenType::END_OF_FILE, .text = "?", .line = line, .column = startCol };
        }
    }
};

// ---------- 3. AST 节点 ----------
struct Expression {
    virtual ~Expression() = default;
    Expression() = default;
    Expression(const Expression&) = default;
    auto operator=(const Expression&) -> Expression& = default;
    Expression(Expression&&) = default;
    auto operator=(Expression&&) -> Expression& = default;
    [[nodiscard]] virtual auto getType() const -> std::string = 0;
    [[nodiscard]] virtual auto toString() const -> std::string = 0;
};

// 字面量
struct Literal : Expression {
    enum class Type : char { T_NUMBER,
                             T_STRING,
                             T_BOOLEAN,
                             T_NIL };
    Type litType;
    std::string value;
    Literal(Type t, std::string v)
        : litType(t)
        , value(std::move(v))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override
    {
        switch (litType) {
        case Type::T_NUMBER:
            return "Number";
        case Type::T_STRING:
            return "String";
        case Type::T_BOOLEAN:
            return "Boolean";
        case Type::T_NIL:
            return "Nil";
        }
        return "Literal";
    }
    [[nodiscard]] auto toString() const -> std::string override
    {
        if (litType == Type::T_STRING) {
            std::string escaped = value;
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\\\"");
                pos += 2;
            }
            return "\"" + escaped + "\"";
        }
        return value;
    }
};

struct Variable : Expression {
    std::string name;
    Variable(std::string n)
        : name(std::move(n))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override { return "Variable"; }
    [[nodiscard]] auto toString() const -> std::string override { return name; }
};

// 字段访问：expr.field 或 expr["field"]（简化，只支持点号）
struct FieldAccess : Expression {
    std::unique_ptr<Expression> object;
    std::string field;
    FieldAccess(std::unique_ptr<Expression> obj, std::string f)
        : object(std::move(obj))
        , field(std::move(f))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override { return "FieldAccess"; }
    [[nodiscard]] auto toString() const -> std::string override
    {
        return object->toString() + "." + field;
    }
};

// 表构造器
struct TableConstructor : Expression {
    std::vector<std::unique_ptr<Expression>> list; // 列表元素
    std::map<std::string, std::unique_ptr<Expression>> fields; // 键值对（简化，键为字符串）
    [[nodiscard]] auto getType() const -> std::string override { return "Table"; }
    [[nodiscard]] auto toString() const -> std::string override
    {
        std::string s = "{";
        for (auto& expr : list) {
            s += expr->toString() + ",";
        }
        for (auto& kv : fields) {
            s += kv.first + "=" + kv.second->toString() + ",";
        }
        if (!list.empty() || !fields.empty())
            s.pop_back();
        s += "}";
        return s;
    }
};

struct BinaryOp : Expression {
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    BinaryOp(std::string o, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : op(std::move(o))
        , left(std::move(l))
        , right(std::move(r))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override { return "BinaryOp"; }
    [[nodiscard]] auto toString() const -> std::string override
    {
        return left->toString() + " " + op + " " + right->toString();
    }
};

struct UnaryOp : Expression {
    std::string op;
    std::unique_ptr<Expression> operand;
    UnaryOp(std::string o, std::unique_ptr<Expression> expr)
        : op(std::move(o))
        , operand(std::move(expr))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override { return "UnaryOp"; }
    [[nodiscard]] auto toString() const -> std::string override
    {
        return op + operand->toString();
    }
};

struct CallExpr : Expression {
    std::unique_ptr<Expression> callee; // 可以是一个表达式（如字段访问）
    bool isMethodCall = false; // 冒号调用
    std::vector<std::unique_ptr<Expression>> args;
    [[nodiscard]] auto getType() const -> std::string override { return "Call"; }
    [[nodiscard]] auto toString() const -> std::string override
    {
        std::string s = callee->toString();
        if (isMethodCall)
            s = ":" + s; // 这里简化显示
        s += "(";
        for (auto& a : args) {
            s += a->toString() + ",";
        }
        if (!args.empty())
            s.pop_back();
        s += ")";
        return s;
    }
};

// ---------- 语句节点 ----------
struct Statement { // NOLINT
    virtual ~Statement() = default;
    [[nodiscard]] virtual auto getType() const -> std::string = 0;
};

struct Program : Statement {
    std::vector<std::unique_ptr<Statement>> body;
    [[nodiscard]] auto getType() const -> std::string override { return "Program"; }
};

struct FunctionDef : Statement {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Statement>> body;
    [[nodiscard]] auto getType() const -> std::string override { return "FunctionDef"; }
};

struct IfStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> thenBody;
    [[nodiscard]] auto getType() const -> std::string override { return "If"; }
};

struct WhileStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;
    [[nodiscard]] auto getType() const -> std::string override { return "While"; }
};

struct ReturnStmt : Statement {
    std::unique_ptr<Expression> value;
    [[nodiscard]] auto getType() const -> std::string override { return "Return"; }
};

struct ExpressionStmt : Statement {
    std::unique_ptr<Expression> expr;
    ExpressionStmt(std::unique_ptr<Expression> e)
        : expr(std::move(e))
    {
    }
    [[nodiscard]] auto getType() const -> std::string override { return "ExpressionStmt"; }
};

struct AssignStmt : Statement {
    std::vector<std::string> vars; // 支持多重赋值，这里简化
    std::unique_ptr<Expression> value;
    [[nodiscard]] auto getType() const -> std::string override { return "Assign"; }
};

struct LocalDecl : Statement {
    std::vector<std::string> vars;
    std::unique_ptr<Expression> init; // 可选初始值
    [[nodiscard]] auto getType() const -> std::string override { return "LocalDecl"; }
};

struct ForLoop : Statement {
    std::string var;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::unique_ptr<Expression> step; // 可选
    std::vector<std::unique_ptr<Statement>> body;
    [[nodiscard]] auto getType() const -> std::string override { return "ForLoop"; }
};

struct ForInLoop : Statement {
    std::vector<std::string> vars;
    std::unique_ptr<Expression> iterable; // 表达式（如 pairs(t)）
    std::vector<std::unique_ptr<Statement>> body;
    [[nodiscard]] auto getType() const -> std::string override { return "ForInLoop"; }
};

struct BreakStmt : Statement {
    [[nodiscard]] auto getType() const -> std::string override { return "Break"; }
};

// ---------- 4. 语法解析器 ----------
class Parser {
public:
    Parser(const std::vector<Token>& tokens)
        : tokens(tokens)
    {
    }

    auto parseProgram() -> std::unique_ptr<Program>
    {
        auto prog = std::make_unique<Program>();
        while (!match(TokenType::END_OF_FILE)) {
            auto stmt = parseStatement();
            if (stmt)
                prog->body.push_back(std::move(stmt));
        }
        return prog;
    }

private:
    std::vector<Token> tokens;
    size_t idx { 0 };

    [[nodiscard]] auto peek() const -> Token { return tokens.at(idx); }
    auto advance() -> Token { return tokens.at(idx++); }
    auto match(TokenType type) -> bool
    {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }
    auto expect(TokenType type) -> bool
    {
        if (match(type))
            return true;
        return false;
    }

    // ---------- 表达式解析（优先级爬升）----------
    auto parseExpression(int minPrec = 0) -> std::unique_ptr<Expression>
    {
        auto left = parsePrefix();
        if (!left)
            return nullptr;

        while (true) {
            Token op = peek();
            int prec = getPrecedence(op.type);
            if (prec < minPrec || !isBinaryOp(op.type))
                break;
            advance();
            auto right = parseExpression(prec + 1);
            if (!right)
                return nullptr;
            left = std::make_unique<BinaryOp>(op.text, std::move(left), std::move(right));
        }
        return left;
    }

    // 解析基本表达式，并处理后续的点号和冒号（字段访问和调用）
    auto parsePrefix() -> std::unique_ptr<Expression>
    {
        auto expr = parsePrimary();
        if (!expr)
            return nullptr;

        while (true) {
            // 点号访问：expr.field
            if (match(TokenType::SYMBOL_DOT)) {
                if (peek().type == TokenType::IDENTIFIER) {
                    std::string field = advance().text;
                    expr = std::make_unique<FieldAccess>(std::move(expr), field);
                } else {
                    // 错误，跳过
                    return expr;
                }
            }
            // 冒号调用：expr:method(...)
            else if (match(TokenType::SYMBOL_COLON)) {
                if (peek().type == TokenType::IDENTIFIER) {
                    std::string method = advance().text;
                    expect(TokenType::SYMBOL_LPAREN);
                    auto call = std::make_unique<CallExpr>();
                    call->callee = std::make_unique<FieldAccess>(std::move(expr), method);
                    call->isMethodCall = true;
                    // 解析参数
                    if (peek().type != TokenType::SYMBOL_RPAREN) {
                        auto arg = parseExpression();
                        if (arg)
                            call->args.push_back(std::move(arg));
                        while (match(TokenType::SYMBOL_COMMA)) {
                            auto arg = parseExpression();
                            if (arg)
                                call->args.push_back(std::move(arg));
                        }
                    }
                    expect(TokenType::SYMBOL_RPAREN);
                    expr = std::move(call);
                } else {
                    return expr;
                }
            }
            // 函数调用：expr(...)    （expr 可以是任何表达式，如字段访问）
            else if (match(TokenType::SYMBOL_LPAREN)) {
                auto call = std::make_unique<CallExpr>();
                call->callee = std::move(expr);
                // 解析参数
                if (peek().type != TokenType::SYMBOL_RPAREN) {
                    auto arg = parseExpression();
                    if (arg)
                        call->args.push_back(std::move(arg));
                    while (match(TokenType::SYMBOL_COMMA)) {
                        auto arg = parseExpression();
                        if (arg)
                            call->args.push_back(std::move(arg));
                    }
                }
                expect(TokenType::SYMBOL_RPAREN);
                expr = std::move(call);
            } else {
                break;
            }
        }
        return expr;
    }

    // 解析原子表达式（字面量、标识符、括号、表构造器）
    auto parsePrimary() -> std::unique_ptr<Expression>
    {
        Token tok = peek();
        if (tok.type == TokenType::NUMBER) {
            advance();
            return std::make_unique<Literal>(Literal::Type::T_NUMBER, tok.text);
        } else if (tok.type == TokenType::STRING) {
            advance();
            return std::make_unique<Literal>(Literal::Type::T_STRING, tok.text);
        } else if (tok.type == TokenType::KEYWORD_TRUE) {
            advance();
            return std::make_unique<Literal>(Literal::Type::T_BOOLEAN, "true");
        } else if (tok.type == TokenType::KEYWORD_FALSE) {
            advance();
            return std::make_unique<Literal>(Literal::Type::T_BOOLEAN, "false");
        } else if (tok.type == TokenType::KEYWORD_NIL) {
            advance();
            return std::make_unique<Literal>(Literal::Type::T_NIL, "nil");
        } else if (tok.type == TokenType::IDENTIFIER) {
            advance();
            return std::make_unique<Variable>(tok.text);
        } else if (tok.type == TokenType::SYMBOL_LPAREN) {
            advance(); // '('
            auto expr = parseExpression();
            expect(TokenType::SYMBOL_RPAREN);
            return expr;
        } else if (tok.type == TokenType::SYMBOL_LCURLY) {
            return parseTableConstructor();
        } else if (tok.type == TokenType::KEYWORD_NOT || tok.type == TokenType::SYMBOL_MINUS) {
            advance();
            auto operand = parseExpression(8);
            if (!operand)
                return nullptr;
            return std::make_unique<UnaryOp>(tok.text, std::move(operand));
        }
        return nullptr;
    }

    // 表构造器
    auto parseTableConstructor() -> std::unique_ptr<Expression>
    {
        advance(); // '{'
        auto table = std::make_unique<TableConstructor>();
        while (peek().type != TokenType::SYMBOL_RCURLY && peek().type != TokenType::END_OF_FILE) {
            // 检测键值对： identifier '=' expr
            if (peek().type == TokenType::IDENTIFIER && tokens.at(idx + 1).type == TokenType::SYMBOL_ASSIGN) {
                std::string key = advance().text;
                advance(); // '='
                auto value = parseExpression();
                if (value)
                    table->fields[key] = std::move(value);
            } else {
                // 列表元素
                auto expr = parseExpression();
                if (expr)
                    table->list.push_back(std::move(expr));
            }
            if (match(TokenType::SYMBOL_COMMA) || match(TokenType::SYMBOL_SEMICOLON)) {
                // 继续
            } else {
                break;
            }
        }
        expect(TokenType::SYMBOL_RCURLY);
        return table;
    }

    auto isBinaryOp(TokenType t) -> bool
    {
        switch (t) {
        case TokenType::SYMBOL_PLUS:
        case TokenType::SYMBOL_MINUS:
        case TokenType::SYMBOL_STAR:
        case TokenType::SYMBOL_SLASH:
        case TokenType::SYMBOL_PERCENT:
        case TokenType::SYMBOL_CARET:
        case TokenType::SYMBOL_EQ:
        case TokenType::SYMBOL_NE:
        case TokenType::SYMBOL_LT:
        case TokenType::SYMBOL_GT:
        case TokenType::SYMBOL_LE:
        case TokenType::SYMBOL_GE:
        case TokenType::SYMBOL_CONCAT:
        case TokenType::KEYWORD_AND:
        case TokenType::KEYWORD_OR:
            return true;
        default:
            return false;
        }
    }

    auto getPrecedence(TokenType t) -> int
    {
        switch (t) {
        case TokenType::KEYWORD_OR:
            return 1;
        case TokenType::KEYWORD_AND:
            return 2;
        case TokenType::SYMBOL_EQ:
        case TokenType::SYMBOL_NE:
        case TokenType::SYMBOL_LT:
        case TokenType::SYMBOL_GT:
        case TokenType::SYMBOL_LE:
        case TokenType::SYMBOL_GE:
            return 3;
        case TokenType::SYMBOL_CONCAT:
            return 4;
        case TokenType::SYMBOL_PLUS:
        case TokenType::SYMBOL_MINUS:
            return 5;
        case TokenType::SYMBOL_STAR:
        case TokenType::SYMBOL_SLASH:
        case TokenType::SYMBOL_PERCENT:
            return 6;
        case TokenType::SYMBOL_CARET:
            return 7;
        default:
            return 0;
        }
    }

    // ---------- 语句解析 ----------
    auto parseStatement() -> std::unique_ptr<Statement>
    {
        Token tok = peek();
        if (tok.type == TokenType::KEYWORD_FUNCTION) {
            return parseFunctionDef();
        } else if (tok.type == TokenType::KEYWORD_LOCAL) {
            return parseLocalDecl();
        } else if (tok.type == TokenType::KEYWORD_IF) {
            return parseIf();
        } else if (tok.type == TokenType::KEYWORD_WHILE) {
            return parseWhile();
        } else if (tok.type == TokenType::KEYWORD_FOR) {
            return parseFor();
        } else if (tok.type == TokenType::KEYWORD_RETURN) {
            return parseReturn();
        } else if (tok.type == TokenType::KEYWORD_BREAK) {
            advance();
            return std::make_unique<BreakStmt>();
        } else if (tok.type == TokenType::IDENTIFIER) {
            return parseAssignmentOrCall();
        }
        advance(); // skip unknown
        return nullptr;
    }

    auto parseFunctionDef() -> std::unique_ptr<FunctionDef>
    {
        advance(); // function
        auto func = std::make_unique<FunctionDef>();
        func->name = advance().text;
        expect(TokenType::SYMBOL_LPAREN);
        if (peek().type != TokenType::SYMBOL_RPAREN) {
            func->params.push_back(advance().text);
            while (match(TokenType::SYMBOL_COMMA)) {
                func->params.push_back(advance().text);
            }
        }
        expect(TokenType::SYMBOL_RPAREN);

        while (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
            auto stmt = parseStatement();
            if (stmt)
                func->body.push_back(std::move(stmt));
        }
        expect(TokenType::KEYWORD_END);
        return func;
    }

    auto parseLocalDecl() -> std::unique_ptr<Statement>
    {
        advance(); // local
        auto decl = std::make_unique<LocalDecl>();
        if (peek().type != TokenType::IDENTIFIER) {
            // 错误
            return decl;
        }
        decl->vars.push_back(advance().text);
        while (match(TokenType::SYMBOL_COMMA)) {
            if (peek().type == TokenType::IDENTIFIER)
                decl->vars.push_back(advance().text);
            else
                break;
        }
        if (match(TokenType::SYMBOL_ASSIGN)) {
            decl->init = parseExpression();
        }
        return decl;
    }

    auto parseAssignmentOrCall() -> std::unique_ptr<Statement>
    {
        auto expr = parseExpression();
        if (!expr)
            return nullptr;
        if (match(TokenType::SYMBOL_ASSIGN)) {
            auto assign = std::make_unique<AssignStmt>();
            if (auto var = dynamic_cast<Variable*>(expr.get())) {
                assign->vars.push_back(var->name);
            } else if (auto field = dynamic_cast<FieldAccess*>(expr.get())) {
                assign->vars.push_back(field->toString());
            } else {
                return nullptr;
            }
            assign->value = parseExpression();
            return assign;
        } else {
            if (dynamic_cast<CallExpr*>(expr.get())) {
                return std::make_unique<ExpressionStmt>(std::move(expr));
            }
            return nullptr;
        }
    }

    auto parseIf() -> std::unique_ptr<IfStmt>
    {
        advance(); // if
        auto ifstmt = std::make_unique<IfStmt>();
        ifstmt->condition = parseExpression();
        expect(TokenType::KEYWORD_THEN);
        while (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
            auto stmt = parseStatement();
            if (stmt)
                ifstmt->thenBody.push_back(std::move(stmt));
        }
        expect(TokenType::KEYWORD_END);
        return ifstmt;
    }

    auto parseWhile() -> std::unique_ptr<WhileStmt>
    {
        advance(); // while
        auto whilestmt = std::make_unique<WhileStmt>();
        whilestmt->condition = parseExpression();
        expect(TokenType::KEYWORD_DO);
        while (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
            auto stmt = parseStatement();
            if (stmt)
                whilestmt->body.push_back(std::move(stmt));
        }
        expect(TokenType::KEYWORD_END);
        return whilestmt;
    }

    auto parseFor() -> std::unique_ptr<Statement>
    {
        advance(); // for
        // 检测是数值for还是通用for
        if (peek().type == TokenType::IDENTIFIER) {
            std::string var = advance().text;
            // 如果下一个是 '='，则是数值for
            if (match(TokenType::SYMBOL_ASSIGN)) {
                auto loop = std::make_unique<ForLoop>();
                loop->var = var;
                loop->start = parseExpression();
                expect(TokenType::SYMBOL_COMMA);
                loop->end = parseExpression();
                if (match(TokenType::SYMBOL_COMMA)) {
                    loop->step = parseExpression();
                }
                expect(TokenType::KEYWORD_DO);
                while (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
                    auto stmt = parseStatement();
                    if (stmt)
                        loop->body.push_back(std::move(stmt));
                }
                expect(TokenType::KEYWORD_END);
                return loop;
            } else {
                // 通用for: for var1, var2 in expr do ... end
                auto loop = std::make_unique<ForInLoop>();
                loop->vars.push_back(var);
                while (match(TokenType::SYMBOL_COMMA)) {
                    if (peek().type == TokenType::IDENTIFIER)
                        loop->vars.push_back(advance().text);
                    else
                        break;
                }
                expect(TokenType::KEYWORD_IN);
                loop->iterable = parseExpression();
                expect(TokenType::KEYWORD_DO);
                while (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
                    auto stmt = parseStatement();
                    if (stmt)
                        loop->body.push_back(std::move(stmt));
                }
                expect(TokenType::KEYWORD_END);
                return loop;
            }
        }
        return nullptr;
    }

    auto parseReturn() -> std::unique_ptr<ReturnStmt>
    {
        advance(); // return
        auto ret = std::make_unique<ReturnStmt>();
        if (peek().type != TokenType::KEYWORD_END && peek().type != TokenType::END_OF_FILE) {
            ret->value = parseExpression();
        }
        return ret;
    }
};

// ---------- 5. DOT 导出器 ----------
class DotExporter {
public:
    static auto exportToDot(const Program* prog, const std::string& dotFilename) -> void
    {
        std::ofstream file(dotFilename);
        file << "digraph AST {\n";
        file << "  node [shape=box, style=filled, fillcolor=lightblue];\n";
        int counter = 0;
        file << "  n0 [label=\"Program\"];\n";
        for (auto& stmt : prog->body) {
            int startId = counter + 1;
            int childId = exportStatement(file, stmt.get(), startId);
            counter = startId;
            file << "  n0 -> n" << childId << ";\n";
        }
        file << "}\n";
        file.close();
    }

    static auto generatePNG(const std::string& dotFilename, const std::string& pngFilename) -> void
    {
        std::string cmd = "dot -Tpng " + dotFilename + " -o " + pngFilename;
        int ret = system(cmd.c_str()); // NOLINT
        if (ret == 0)
            std::cout << "图片已生成: " << pngFilename << '\n';
        else
            std::cerr << "生成图片失败，请确保已安装 Graphviz (dot 命令可用)\n";
    }

private:
    static auto escapeLabel(const std::string& s) -> std::string
    {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find('"', pos)) != std::string::npos) {
            result.replace(pos, 1, "\\\"");
            pos += 2;
        }
        return result;
    }
    static auto exportStatement(std::ofstream& file, const Statement* stmt, int& id) -> int
    {
        int currentId = id++;
        std::string label;

        if (auto f = dynamic_cast<const FunctionDef*>(stmt)) {
            label = "FunctionDef\\n" + f->name + "(";
            for (auto& p : f->params)
                label += p + ",";
            if (!f->params.empty())
                label.pop_back();
            label += ")";
            file << "  n" << currentId << " [label=\"" << label << "\"];\n";
            for (auto& child : f->body) {
                int childId = exportStatement(file, child.get(), id);
                file << "  n" << currentId << " -> n" << childId << ";\n";
            }
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto i = dynamic_cast<const IfStmt*>(stmt)) {
            label = "If\\n" + (i->condition ? i->condition->toString() : "");
            file << "  n" << currentId << " [label=\"" << label << "\"];\n";
            for (auto& child : i->thenBody) {
                int childId = exportStatement(file, child.get(), id);
                file << "  n" << currentId << " -> n" << childId << ";\n";
            }
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto w = dynamic_cast<const WhileStmt*>(stmt)) {
            label = "While\\n" + (w->condition ? w->condition->toString() : "");
            file << "  n" << currentId << " [label=\"" << label << "\"];\n";
            for (auto& child : w->body) {
                int childId = exportStatement(file, child.get(), id);
                file << "  n" << currentId << " -> n" << childId << ";\n";
            }
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto r = dynamic_cast<const ReturnStmt*>(stmt)) {
            label = "Return\\n" + (r->value ? r->value->toString() : "");
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto a = dynamic_cast<const AssignStmt*>(stmt)) {
            std::string vars;
            for (auto& v : a->vars)
                vars += v + ",";
            if (!vars.empty())
                vars.pop_back();
            label = "Assign\\n" + vars + " = " + (a->value ? a->value->toString() : "");
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto l = dynamic_cast<const LocalDecl*>(stmt)) {
            std::string vars;
            for (auto& v : l->vars)
                vars += v + ",";
            if (!vars.empty())
                vars.pop_back();
            label = "LocalDecl\\n" + vars + (l->init ? " = " + l->init->toString() : "");
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto fl = dynamic_cast<const ForLoop*>(stmt)) {
            label = "ForLoop\\n" + fl->var + " = " + (fl->start ? fl->start->toString() : "") + " to " + (fl->end ? fl->end->toString() : "");
            if (fl->step)
                label += " step " + fl->step->toString();
            file << "  n" << currentId << " [label=\"" << label << "\"];\n";
            for (auto& child : fl->body) {
                int childId = exportStatement(file, child.get(), id);
                file << "  n" << currentId << " -> n" << childId << ";\n";
            }
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto fl = dynamic_cast<const ForInLoop*>(stmt)) {
            std::string vars;
            for (auto& v : fl->vars)
                vars += v + ",";
            if (!vars.empty())
                vars.pop_back();
            label = "ForInLoop\\n" + vars + " in " + (fl->iterable ? fl->iterable->toString() : "");
            file << "  n" << currentId << " [label=\"" << label << "\"];\n";
            for (auto& child : fl->body) {
                int childId = exportStatement(file, child.get(), id);
                file << "  n" << currentId << " -> n" << childId << ";\n";
            }
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (dynamic_cast<const BreakStmt*>(stmt)) {
            label = "Break";
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        } else if (auto es = dynamic_cast<const ExpressionStmt*>(stmt)) {
            label = "ExprStmt\\n" + (es->expr ? es->expr->toString() : "");
            file << "  n" << currentId << " [label=\"" << escapeLabel(label) << "\"];\n";
        }
        return currentId;
    }
};

} // anonymous namespace

// ---------- 导出的 API ----------
export namespace Engine::Basics::Lua::ASTGen {

auto GenAST() -> void
{
    // 使用您提供的测试代码（已包含在文件中）
    std::string code = R"(
local math = require("math")
print("1156137")
bg = os.time()
i=0;
snd.play_sfx("__Engine_SFX__@annc.mp3")
elapsed = 0
b={"手感不好","网卡","断触","卡键","手抖","误触","帧率低","高延迟","瓶颈期","没手感","没心态","没状态","刚睡醒","没支架","打累了","没散热器","卡帧掉帧","手冷手汗","周围太吵","外卖到了","手冻僵了","在上厕所","预判失误","随便玩玩","我在上课","没戴指套","没戴耳机","屏幕太滑了","手机太烫了","腱鞘炎犯了","天气太冷了","对面开挂了","好久没玩了","我妈叫我了","耳机没电了","电量低提示了","灵敏度没改","上线第一把","刚回游不会玩","按键有问题","打电话听不到声音","皮肤手感不行","充电影响发挥","有人打电话给我","有人发消息不小心点到了","新换的键位不熟悉","边吃饭边打的","刚刚在回信息","我这边好吵","坐久了肩酸","手机没平板好用","刚刚黑客入侵了"}
local my_name = dm.getworkername()
print("[worker] My name: " .. my_name)
local child_handle = dm.spawn_worker("child_worker_renderer", "__Engine_Test_Worker__@worker_renderer.lua")
if child_handle then
    print("[worker] Child worker created, name: " .. child_handle.name())
else
    print("[worker] Failed to create child worker")
end
while true do
    local dt = sleep_frame()
    if dt == 0.0 then
        break
    end
    elapsed=elapsed+dt
    local keystats = getkeystats()
    if keystats.mouse_left then
        gui.debug_text("LMB down", 0, 60, 255, 255, 255, 255, 2, 114514)
    end
    if keystats.a then
        gui.debug_text("A down", 0, 80, 255, 255, 255, 255, 2, 114514)
        end
        gui.rect(0, 0, 640, 480, 255, 0, 0, 255, 10)
        gui.debug_text("wa da xi wa L de su",0,0,255,0,0,255,4,114514)
        gui.debug_text("zhe hang zi shi lua hui zhi chu lai de",0,400,255,0,0,255,3,114514)
        gui.debug_text(tostring(dt) .. "," .. tostring(os.time()),0,100,255,0,0,255,2,114514)
        red = 0.5 + 0.5 * math.sin(elapsed)
        green = 0.5 + 0.5 * math.sin(elapsed + math.pi * 2 / 3)
        blue = 0.5 + 0.5 * math.sin(elapsed + math.pi * 4 / 3)
        gui.debug_text(tostring(os.time()-
        bg),math.sin(elapsed)*200+320,math.cos(elapsed)*200+240,math.floor(red*255),math.floor(green*255),math.floor(blue*255),255,5,114514)
        gui.text("t11est", "__Engine_Font__@SourceHanSans", math.sin(elapsed)*200+320, math.cos(elapsed)*200+240, math.floor(red*255),math.floor(green*255),math.floor(blue*255), 255, math.floor((red+math.pi /2)*255),math.floor((green+math.pi /2)*255),math.floor((blue+math.pi /2)*255), 255, math.floor(math.abs(math.floor(red*20)+1)), 1,0,0,0, 999999)
        gui.text(b[math.random(1, #b)], "__Engine_Font__@SourceHanSans", math.sin(elapsed)*100+320, math.cos(elapsed)*100+240, math.floor(red*255),math.floor(green*255),math.floor(blue*255), 255, math.floor((red+math.pi /2)*255),math.floor((green+math.pi /2)*255),math.floor((blue+math.pi /2)*255), 255, 70, 3,math.abs(math.floor(red*360)+1),math.floor(blue*255),math.floor(green*20), 999999)
        gui.text(tostring(i), "__Engine_Font__@SourceHanSans", 0, 40, 0,0,0, 255, 0,0,0, 255, 20, 0,0,0,0, 999999)
        vec=dm.getlist()
        for i, v in pairs(vec) do
            gui.text(v, "__Engine_Font__@SourceHanSans", 0, (i-1)*20, 0,0,0, 255, 0,0,0, 255, 20, 0,0,0,0, 999999)
            end
            gui.draw_svg("__Engine_StartUp__@note.svg", 80, 50, math.abs(math.floor(red*500)+1), math.abs(math.floor(red*10)+300), math.abs(math.cos(elapsed)*20),  math.abs(math.cos(elapsed*10)*200), math.abs(math.sin(elapsed*7)*200), 1919810)
            i=i+1
            end
            print("myGO\n")
    )";

    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto ast = parser.parseProgram();
    const std::string dotFile = "ast.dot";
    const std::string pngFile = "ast.png";

    DotExporter::exportToDot(ast.get(), dotFile);
    DotExporter::generatePNG(dotFile, pngFile);

    std::cout << "AST generation test completed.\n";
}

} // namespace Engine::Basics::Lua::ASTGen
