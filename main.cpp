#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <cctype>

// =======================================================
// ERROR HANDLING MECHANISM
// =======================================================

int current_line = 1; 
int current_column = 1;

class ULangError : public std::runtime_error {
public:
    int line;
    int column;
    std::string type;

    ULangError(const std::string& message, const std::string& error_type, int err_line, int err_col)
        : std::runtime_error(message), line(err_line), column(err_col), type(error_type) {}
    
    std::string getFullMessage() const {
        return "ERROR [" + type + "] Line " + std::to_string(line) + ", Column " + std::to_string(column) + ": " + what();
    }
};

inline void throw_lexer_error(const std::string& msg) {
    throw ULangError(msg, "Lexer", current_line, current_column);
}

inline void throw_parser_error(const std::string& msg) {
    throw ULangError(msg, "Parser", current_line, current_column);
}

inline void throw_runtime_error(const std::string& msg) {
    throw ULangError(msg, "Runtime", current_line, current_column);
}

// =======================================================
// TOKEN AND LEXER
// =======================================================

enum TokenType { 
    TOK_ID, TOK_NUMBER, TOK_STRING_LIT, TOK_LPAREN, TOK_RPAREN, TOK_COMMA, TOK_EQUALS,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_LT, TOK_GT, TOK_EE, TOK_NE, TOK_LBRACE, TOK_RBRACE,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_IN, TOK_THAT, TOK_CASE,
    TOK_EOF 
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int column;

    Token(TokenType t, const std::string& txt, int l = 0, int c = 0) 
        : type(t), text(txt), line(l), column(c) {}
};

TokenType check_keyword(const std::string& text) {
    if (text == "if") return TOK_IF;
    if (text == "else") return TOK_ELSE;
    if (text == "while") return TOK_WHILE;
    if (text == "in") return TOK_IN;
    if (text == "that") return TOK_THAT;
    if (text == "case") return TOK_CASE;
    return TOK_ID;
}

std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    current_line = 1;
    current_column = 1;

    while (i < source.length()) {
        char c = source[i];

        if (c == '\n') { 
            i++;
            current_line++;
            current_column = 1;
            continue; 
        }

        if (isspace(c)) { 
            i++; 
            current_column++;
            continue; 
        }

        int token_start_line = current_line;
        int token_start_column = current_column;

        if (c == '"') {
            i++; 
            current_column++;
            std::string s;
            while (i < source.length() && source[i] != '"') {
                if (source[i] == '\n') {
                    current_line++;
                    current_column = 1;
                } else {
                    current_column++;
                }
                s += source[i++];
            }
            if (i >= source.length()) {
                throw_lexer_error("Unclosed string literal.");
            }
            i++; 
            current_column++;
            tokens.push_back(Token(TOK_STRING_LIT, s, token_start_line, token_start_column));
        } 
        else if (isalpha(c) || c == '_') {
            std::string id;
            while (i < source.length() && (isalnum(source[i]) || source[i] == '_')) {
                id += source[i++];
            }
            tokens.push_back(Token(check_keyword(id), id, token_start_line, token_start_column));
            current_column += id.length();
        } 
        else if (isdigit(c)) {
            std::string num;
            while (i < source.length() && (isdigit(source[i]) || source[i] == '.')) {
                num += source[i++];
            }
            tokens.push_back(Token(TOK_NUMBER, num, token_start_line, token_start_column));
            current_column += num.length();
        } 
        else if (c == '=') {
            if (i + 1 < source.length() && source[i+1] == '=') { 
                tokens.push_back(Token(TOK_EE, "==", token_start_line, token_start_column)); 
                i+=2; 
                current_column+=2;
            } else { 
                tokens.push_back(Token(TOK_EQUALS, "=", token_start_line, token_start_column)); 
                i++; 
                current_column++;
            }
        } 
        else if (c == '!') {
            if (i + 1 < source.length() && source[i+1] == '=') {
                tokens.push_back(Token(TOK_NE, "!=", token_start_line, token_start_column));
                i+=2;
                current_column+=2;
            } else {
                i++;
                current_column++;
                throw_lexer_error("Unknown operator: '!'. Only '!=' is supported.");
            }
        }
        else if (c == '(') { tokens.push_back(Token(TOK_LPAREN, "(", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == ')') { tokens.push_back(Token(TOK_RPAREN, ")", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '{') { tokens.push_back(Token(TOK_LBRACE, "{", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '}') { tokens.push_back(Token(TOK_RBRACE, "}", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == ',') { tokens.push_back(Token(TOK_COMMA, ",", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '+') { tokens.push_back(Token(TOK_PLUS, "+", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '-') { tokens.push_back(Token(TOK_MINUS, "-", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '*') { tokens.push_back(Token(TOK_STAR, "*", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '/') { tokens.push_back(Token(TOK_SLASH, "/", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '<') { tokens.push_back(Token(TOK_LT, "<", token_start_line, token_start_column)); i++; current_column++; }
        else if (c == '>') { tokens.push_back(Token(TOK_GT, ">", token_start_line, token_start_column)); i++; current_column++; }
        
        else { 
            i++; 
            current_column++;
            throw_lexer_error("Unknown character."); 
        }
    }
    tokens.push_back(Token(TOK_EOF, "", current_line, current_column));
    return tokens;
}

// =======================================================
// ULANG OBJECT AND AST DEFINITIONS
// =======================================================

class Interpreter;
class ULangObject;
class FunctionObject;

class ULangObject {
public:
    enum Type { NUMBER, STRING, BOOLEAN, FUNCTION, VOID };
    Type type;
    ULangObject(Type t) : type(t) {}
    virtual ~ULangObject() = default;
    virtual std::string toString() const = 0;
};

class NumberObject : public ULangObject {
public:
    double value;
    NumberObject(double val) : ULangObject(NUMBER), value(val) {}
    std::string toString() const override { 
        std::string s = std::to_string(value);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
};

class BooleanObject : public ULangObject {
public:
    bool value;
    BooleanObject(bool val) : ULangObject(BOOLEAN), value(val) {}
    std::string toString() const override { return value ? "true" : "false"; }
};

class StringObject : public ULangObject {
public:
    std::string value;
    StringObject(const std::string& val) : ULangObject(STRING), value(val) {}
    std::string toString() const override { return value; }
};

class VoidObject : public ULangObject {
public:
    VoidObject() : ULangObject(VOID) {}
    std::string toString() const override { return "void"; }
};

static std::shared_ptr<ULangObject> VOID_INSTANCE = std::make_shared<VoidObject>();
static std::shared_ptr<ULangObject> TRUE_OBJECT = std::make_shared<BooleanObject>(true);
static std::shared_ptr<ULangObject> FALSE_OBJECT = std::make_shared<BooleanObject>(false);

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) = 0;
};

class NumberLiteral : public ASTNode {
public:
    double value;
    NumberLiteral(double val) : value(val) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class StringLiteral : public ASTNode {
public:
    std::string value;
    StringLiteral(const std::string& val) : value(val) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class Identifier : public ASTNode {
public:
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class BinaryOp : public ASTNode {
public:
    std::string op;
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;
    BinaryOp(const std::string& o, std::shared_ptr<ASTNode> l, std::shared_ptr<ASTNode> r)
        : op(o), left(l), right(r) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class Block : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> statements;
    Block(const std::vector<std::shared_ptr<ASTNode>>& stmts) : statements(stmts) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class IfStatement : public ASTNode {
public:
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<ASTNode> thenBlock;
    std::shared_ptr<ASTNode> elseBlock;
    IfStatement(std::shared_ptr<ASTNode> cond, std::shared_ptr<ASTNode> thenB, std::shared_ptr<ASTNode> elseB = nullptr)
        : condition(cond), thenBlock(thenB), elseBlock(elseB) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class WhileLoop : public ASTNode {
public:
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<ASTNode> body;
    WhileLoop(std::shared_ptr<ASTNode> cond, std::shared_ptr<ASTNode> bodyB)
        : condition(cond), body(bodyB) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class Assignment : public ASTNode {
public:
    std::string name;
    std::shared_ptr<ASTNode> value;
    Assignment(const std::string& n, std::shared_ptr<ASTNode> v) : name(n), value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class FunctionCall : public ASTNode {
public:
    std::string name;
    std::vector<std::shared_ptr<ASTNode>> arguments;
    FunctionCall(const std::string& n, const std::vector<std::shared_ptr<ASTNode>>& args)
        : name(n), arguments(args) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override;
};

class FunctionObject : public ULangObject {
public:
    std::vector<std::string> parameters;
    std::vector<std::shared_ptr<ASTNode>> body;
    
    FunctionObject(const std::vector<std::string>& params, const std::vector<std::shared_ptr<ASTNode>>& b)
        : ULangObject(FUNCTION), parameters(params), body(b) {}
    
    std::string toString() const override { return "<function>"; }
    
    virtual std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args);
};

// =======================================================
// INTERPRETER AND EVALUATION METHODS
// =======================================================

class Interpreter {
private:
    std::map<std::string, std::shared_ptr<ULangObject>> environment;

public:
    Interpreter() = default;

    void define(const std::string& name, std::shared_ptr<ULangObject> obj) {
        environment[name] = obj;
    }

    std::shared_ptr<ULangObject> lookup(const std::string& name) {
        if (environment.count(name)) {
            return environment.at(name);
        }
        throw_runtime_error("Undefined identifier '" + name + "'");
    }

    std::shared_ptr<ULangObject> execute(const std::vector<std::shared_ptr<ASTNode>>& program) {
        std::shared_ptr<ULangObject> result = VOID_INSTANCE;
        for (const auto& statement : program) {
            result = statement->evaluate(*this);
        }
        return result;
    }
};

std::shared_ptr<ULangObject> NumberLiteral::evaluate(Interpreter& interpreter) {
    return std::make_shared<NumberObject>(value);
}

std::shared_ptr<ULangObject> StringLiteral::evaluate(Interpreter& interpreter) {
    return std::make_shared<StringObject>(value);
}

std::shared_ptr<ULangObject> Identifier::evaluate(Interpreter& interpreter) {
    return interpreter.lookup(name);
}

std::shared_ptr<ULangObject> Assignment::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> val = value->evaluate(interpreter);
    interpreter.define(name, val);
    return val;
}

std::shared_ptr<ULangObject> Block::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> result = VOID_INSTANCE;
    for (const auto& statement : statements) {
        result = statement->evaluate(interpreter);
    }
    return result;
}

std::shared_ptr<ULangObject> BinaryOp::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> leftVal = left->evaluate(interpreter);
    std::shared_ptr<ULangObject> rightVal = right->evaluate(interpreter);

    if (leftVal->type != ULangObject::NUMBER || rightVal->type != ULangObject::NUMBER) {
        throw_runtime_error("Binary operations only support numbers.");
    }
    double l = std::static_pointer_cast<NumberObject>(leftVal)->value;
    double r = std::static_pointer_cast<NumberObject>(rightVal)->value;

    if (op == "+") return std::make_shared<NumberObject>(l + r);
    if (op == "-") return std::make_shared<NumberObject>(l - r);
    if (op == "*") return std::make_shared<NumberObject>(l * r);
    if (op == "/") return std::make_shared<NumberObject>(l / r);
    
    if (op == "==") return l == r ? TRUE_OBJECT : FALSE_OBJECT;
    if (op == "!=") return l != r ? TRUE_OBJECT : FALSE_OBJECT;
    if (op == "<") return l < r ? TRUE_OBJECT : FALSE_OBJECT;
    if (op == ">") return l > r ? TRUE_OBJECT : FALSE_OBJECT;
    
    throw_runtime_error("Unknown binary operator " + op);
}

std::shared_ptr<ULangObject> IfStatement::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> conditionVal = condition->evaluate(interpreter);
    
    if (conditionVal->type != ULangObject::BOOLEAN) {
        throw_runtime_error("If condition must be a boolean.");
    }

    if (std::static_pointer_cast<BooleanObject>(conditionVal)->value) {
        return thenBlock->evaluate(interpreter);
    } else if (elseBlock) {
        return elseBlock->evaluate(interpreter);
    }
    return VOID_INSTANCE;
}

std::shared_ptr<ULangObject> WhileLoop::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> result = VOID_INSTANCE;
    while (true) {
        std::shared_ptr<ULangObject> conditionVal = condition->evaluate(interpreter);
        if (conditionVal->type != ULangObject::BOOLEAN) {
            throw_runtime_error("While condition must be a boolean.");
        }
        if (!std::static_pointer_cast<BooleanObject>(conditionVal)->value) {
            break;
        }
        result = body->evaluate(interpreter);
    }
    return result;
}

std::shared_ptr<ULangObject> FunctionCall::evaluate(Interpreter& interpreter) {
    std::shared_ptr<ULangObject> funcObj = interpreter.lookup(name);
    if (funcObj->type != ULangObject::FUNCTION) throw_runtime_error("'" + name + "' is not a function.");
    std::shared_ptr<FunctionObject> func = std::static_pointer_cast<FunctionObject>(funcObj);
    std::vector<std::shared_ptr<ULangObject>> evaluatedArgs;
    for (const auto& argNode : arguments) evaluatedArgs.push_back(argNode->evaluate(interpreter));
    return func->call(interpreter, evaluatedArgs);
}

std::shared_ptr<ULangObject> FunctionObject::call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) {
    for (size_t i = 0; i < parameters.size() && i < args.size(); ++i) interpreter.define(parameters[i], args[i]);
    return interpreter.execute(body);
}

double get_number_arg_safe(const std::vector<std::shared_ptr<ULangObject>>& args, size_t index, const std::string& funcName) {
    if (args.size() <= index || args[index]->type != ULangObject::NUMBER) {
        throw_runtime_error(funcName + " function expects a number at argument " + std::to_string(index + 1));
    }
    return std::static_pointer_cast<NumberObject>(args[index])->value;
}

std::string get_string_arg_safe(const std::vector<std::shared_ptr<ULangObject>>& args, size_t index, const std::string& funcName) {
    if (args.size() <= index || args[index]->type != ULangObject::STRING) {
        throw_runtime_error(funcName + " function expects a string at argument " + std::to_string(index + 1));
    }
    return std::static_pointer_cast<StringObject>(args[index])->value;
}


void load_libs(Interpreter& interpreter) {
    
    class PrintFunc : public FunctionObject {
    public:
        PrintFunc() : FunctionObject({"msg"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            for(const auto& arg : args) std::cout << arg->toString() << " ";
            std::cout << std::endl;
            return VOID_INSTANCE;
        }
    };
    interpreter.define("print", std::make_shared<PrintFunc>());

    class InputFunc : public FunctionObject {
    public:
        InputFunc() : FunctionObject({"prompt"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            if (!args.empty()) std::cout << args[0]->toString();
            std::string in;
            std::getline(std::cin, in);
            return std::make_shared<StringObject>(in);
        }
    };
    interpreter.define("input", std::make_shared<InputFunc>());
    
    class ToNumberFunc : public FunctionObject {
    public:
        ToNumberFunc() : FunctionObject({"str"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            try {
                std::string s = get_string_arg_safe(args, 0, "to_number"); 
                size_t pos;
                double val = std::stod(s, &pos);
                if (pos != s.length()) throw std::invalid_argument("not a full number");
                return std::make_shared<NumberObject>(val);
            } catch (const std::exception& e) {
                return std::make_shared<NumberObject>(0.0);
            }
        }
    };
    interpreter.define("to_number", std::make_shared<ToNumberFunc>());

    
    class MathAbs : public FunctionObject {
    public:
        MathAbs() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "abs"); 
            return std::make_shared<NumberObject>(std::fabs(x));
        }
    };
    interpreter.define("abs", std::make_shared<MathAbs>());

    class MathCeil : public FunctionObject {
    public:
        MathCeil() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "ceil"); 
            return std::make_shared<NumberObject>(std::ceil(x));
        }
    };
    interpreter.define("ceil", std::make_shared<MathCeil>());

    class MathFloor : public FunctionObject {
    public:
        MathFloor() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "floor"); 
            return std::make_shared<NumberObject>(std::floor(x));
        }
    };
    interpreter.define("floor", std::make_shared<MathFloor>());

    class MathTrunc : public FunctionObject {
    public:
        MathTrunc() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "trunc"); 
            return std::make_shared<NumberObject>(std::trunc(x));
        }
    };
    interpreter.define("trunc", std::make_shared<MathTrunc>());

    class MathRound : public FunctionObject {
    public:
        MathRound() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "round"); 
            return std::make_shared<NumberObject>(std::round(x));
        }
    };
    interpreter.define("round", std::make_shared<MathRound>());

    class MathFmod : public FunctionObject {
    public:
        MathFmod() : FunctionObject({"x", "y"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "fmod"); 
            double y = get_number_arg_safe(args, 1, "fmod"); 
            return std::make_shared<NumberObject>(std::fmod(x, y));
        }
    };
    interpreter.define("fmod", std::make_shared<MathFmod>());

    class MathLog : public FunctionObject {
    public:
        MathLog() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "log");
            return std::make_shared<NumberObject>(std::log(x));
        }
    };
    interpreter.define("log", std::make_shared<MathLog>());

    class MathLog10 : public FunctionObject {
    public:
        MathLog10() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "log10"); 
            return std::make_shared<NumberObject>(std::log10(x));
        }
    };
    interpreter.define("log10", std::make_shared<MathLog10>());

    class MathSin : public FunctionObject {
    public:
        MathSin() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "sin"); 
            return std::make_shared<NumberObject>(std::sin(x));
        }
    };
    interpreter.define("sin", std::make_shared<MathSin>());

    class MathCos : public FunctionObject {
    public:
        MathCos() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "cos"); 
            return std::make_shared<NumberObject>(std::cos(x));
        }
    };
    interpreter.define("cos", std::make_shared<MathCos>());

    class MathTan : public FunctionObject {
    public:
        MathTan() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "tan"); 
            return std::make_shared<NumberObject>(std::tan(x));
        }
    };
    interpreter.define("tan", std::make_shared<MathTan>());
    
    class MathAtan : public FunctionObject {
    public:
        MathAtan() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "atan");
            return std::make_shared<NumberObject>(std::atan(x));
        }
    };
    interpreter.define("atan", std::make_shared<MathAtan>());

    class MathAcos : public FunctionObject {
    public:
        MathAcos() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "acos"); 
            return std::make_shared<NumberObject>(std::acos(x));
        }
    };
    interpreter.define("acos", std::make_shared<MathAcos>());

    class MathAsin : public FunctionObject {
    public:
        MathAsin() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double x = get_number_arg_safe(args, 0, "asin");
            return std::make_shared<NumberObject>(std::asin(x));
        }
    };
    interpreter.define("asin", std::make_shared<MathAsin>());

    class MathPow : public FunctionObject {
    public:
        MathPow() : FunctionObject({"base", "exp"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double b = get_number_arg_safe(args, 0, "pow"); 
            double e = get_number_arg_safe(args, 1, "pow"); 
            return std::make_shared<NumberObject>(std::pow(b, e));
        }
    };
    interpreter.define("pow", std::make_shared<MathPow>());

    class MathSqrt : public FunctionObject {
    public:
        MathSqrt() : FunctionObject({"x"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            double val = get_number_arg_safe(args, 0, "sqrt"); 
            return std::make_shared<NumberObject>(std::sqrt(val));
        }
    };
    interpreter.define("sqrt", std::make_shared<MathSqrt>());

    class StrConcat : public FunctionObject {
    public:
        StrConcat() : FunctionObject({"s1", "s2"}, {}) {} 
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            std::string res = "";
            for(const auto& arg : args) res += arg->toString();
            return std::make_shared<StringObject>(res);
        }
    };
    interpreter.define("concat", std::make_shared<StrConcat>());

    class StrLen : public FunctionObject {
    public:
        StrLen() : FunctionObject({"s"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            std::string s = get_string_arg_safe(args, 0, "len");
            return std::make_shared<NumberObject>((double)s.length());
        }
    };
    interpreter.define("len", std::make_shared<StrLen>());
    
    class StrSubstr : public FunctionObject {
    public:
        StrSubstr() : FunctionObject({"s", "start", "len"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            std::string s = get_string_arg_safe(args, 0, "substr"); 
            double start_d = get_number_arg_safe(args, 1, "substr"); 
            double len_d = get_number_arg_safe(args, 2, "substr"); 
            
            size_t start = (size_t)std::max(0.0, std::round(start_d));
            size_t len = (size_t)std::max(0.0, std::round(len_d));
            
            if (start >= s.length()) return std::make_shared<StringObject>("");
            
            return std::make_shared<StringObject>(s.substr(start, len));
        }
    };
    interpreter.define("substr", std::make_shared<StrSubstr>());

    class StrFind : public FunctionObject {
    public:
        StrFind() : FunctionObject({"s", "search_str"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            std::string s = get_string_arg_safe(args, 0, "find"); 
            std::string search = get_string_arg_safe(args, 1, "find"); 
            
            size_t pos = s.find(search);
            
            if (pos == std::string::npos) return std::make_shared<NumberObject>(-1);
            
            return std::make_shared<NumberObject>((double)pos);
        }
    };
    interpreter.define("find", std::make_shared<StrFind>());

    class StrReplace : public FunctionObject {
    public:
        StrReplace() : FunctionObject({"s", "start", "count", "replacement"}, {}) {}
        std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) override {
            std::string s = get_string_arg_safe(args, 0, "replace");
            double start_d = get_number_arg_safe(args, 1, "replace"); 
            double count_d = get_number_arg_safe(args, 2, "replace"); 
            std::string replacement = get_string_arg_safe(args, 3, "replace"); 

            size_t start = (size_t)std::max(0.0, std::round(start_d));
            size_t count = (size_t)std::max(0.0, std::round(count_d));
            
            if (start > s.length()) start = s.length();
            if (start + count > s.length()) count = s.length() - start;

            s.replace(start, count, replacement);
            return std::make_shared<StringObject>(s);
        }
    };
    interpreter.define("replace", std::make_shared<StrReplace>());
}


// =======================================================
// PARSER IMPLEMENTATION
// =======================================================

std::shared_ptr<ASTNode> parse_expression(const std::vector<Token>& tokens, size_t& current);
std::shared_ptr<ASTNode> parse_comparison(const std::vector<Token>& tokens, size_t& current);
std::shared_ptr<ASTNode> parse_term(const std::vector<Token>& tokens, size_t& current);
std::shared_ptr<ASTNode> parse_factor(const std::vector<Token>& tokens, size_t& current);

void expect(const std::vector<Token>& tokens, size_t& current, TokenType expected) {
    if (tokens[current].type != expected) {
        current_line = tokens[current].line;
        current_column = tokens[current].column;
        
        std::string expectedName;
        if (expected == TOK_LBRACE) expectedName = "'{'";
        else if (expected == TOK_RBRACE) expectedName = "'}'";
        else if (expected == TOK_RPAREN) expectedName = "')'";
        else if (expected == TOK_EOF) expectedName = "end of file";
        else if (expected == TOK_EQUALS) expectedName = "'='";
        else if (expected == TOK_COMMA) expectedName = "','";
        else expectedName = "expected token type";

        throw_parser_error("Unexpected token: '" + tokens[current].text + "'. Expected: " + expectedName);
    }
    current++;
}

std::shared_ptr<ASTNode> parse_block(const std::vector<Token>& tokens, size_t& current) {
    expect(tokens, current, TOK_LBRACE);
    std::vector<std::shared_ptr<ASTNode>> statements;
    while (tokens[current].type != TOK_RBRACE && tokens[current].type != TOK_EOF) {
        statements.push_back(parse_expression(tokens, current));
    }
    expect(tokens, current, TOK_RBRACE);
    return std::make_shared<Block>(statements);
}

std::shared_ptr<ASTNode> parse_if(const std::vector<Token>& tokens, size_t& current) {
    expect(tokens, current, TOK_IF);
    expect(tokens, current, TOK_LPAREN);
    std::shared_ptr<ASTNode> condition = parse_comparison(tokens, current);
    expect(tokens, current, TOK_RPAREN);

    expect(tokens, current, TOK_IN);
    expect(tokens, current, TOK_THAT);
    expect(tokens, current, TOK_CASE);

    std::shared_ptr<ASTNode> thenBlock = parse_block(tokens, current);
    std::shared_ptr<ASTNode> elseBlock = nullptr;

    if (tokens[current].type == TOK_ELSE) {
        current++;
        elseBlock = parse_block(tokens, current);
    }
    return std::make_shared<IfStatement>(condition, thenBlock, elseBlock);
}

std::shared_ptr<ASTNode> parse_while(const std::vector<Token>& tokens, size_t& current) {
    expect(tokens, current, TOK_WHILE);
    expect(tokens, current, TOK_LPAREN);
    std::shared_ptr<ASTNode> condition = parse_comparison(tokens, current);
    expect(tokens, current, TOK_RPAREN);

    std::shared_ptr<ASTNode> body = parse_block(tokens, current);
    return std::make_shared<WhileLoop>(condition, body);
}


std::shared_ptr<ASTNode> parse_expression(const std::vector<Token>& tokens, size_t& current) {
    if (tokens[current].type == TOK_IF) return parse_if(tokens, current);
    if (tokens[current].type == TOK_WHILE) return parse_while(tokens, current);

    if (tokens[current].type == TOK_ID) {
        std::string name = tokens[current].text;
        size_t next_token = current + 1; // Peek ahead

        if (next_token < tokens.size() && tokens[next_token].type == TOK_EQUALS) { 
            current += 2; // Consume ID and =
            std::shared_ptr<ASTNode> value = parse_comparison(tokens, current);
            return std::make_shared<Assignment>(name, value);
        }
        else if (next_token < tokens.size() && tokens[next_token].type == TOK_LPAREN) { 
            current++; // Consume ID
            current++; // Consume (
            
            std::vector<std::shared_ptr<ASTNode>> args;
            while (tokens[current].type != TOK_RPAREN && tokens[current].type != TOK_EOF) {
                args.push_back(parse_comparison(tokens, current));
                if (tokens[current].type == TOK_COMMA) current++;
            }
            expect(tokens, current, TOK_RPAREN);
            return std::make_shared<FunctionCall>(name, args);
        }
    }
    return parse_comparison(tokens, current);
}


std::shared_ptr<ASTNode> parse_comparison(const std::vector<Token>& tokens, size_t& current) {
    std::shared_ptr<ASTNode> left = parse_term(tokens, current);
    while (tokens[current].type == TOK_EE || tokens[current].type == TOK_NE || tokens[current].type == TOK_LT || tokens[current].type == TOK_GT) {
        std::string op = tokens[current].text;
        current++;
        std::shared_ptr<ASTNode> right = parse_term(tokens, current);
        left = std::make_shared<BinaryOp>(op, left, right);
    }
    return left;
}

std::shared_ptr<ASTNode> parse_term(const std::vector<Token>& tokens, size_t& current) {
    std::shared_ptr<ASTNode> left = parse_factor(tokens, current);
    while (tokens[current].type == TOK_PLUS || tokens[current].type == TOK_MINUS) {
        std::string op = tokens[current].text;
        current++;
        std::shared_ptr<ASTNode> right = parse_factor(tokens, current);
        left = std::make_shared<BinaryOp>(op, left, right);
    }
    return left;
}

std::shared_ptr<ASTNode> parse_factor(const std::vector<Token>& tokens, size_t& current) {
    if (tokens[current].type == TOK_NUMBER) {
        double val = std::stod(tokens[current].text);
        current++;
        return std::make_shared<NumberLiteral>(val);
    }
    if (tokens[current].type == TOK_STRING_LIT) {
        std::string val = tokens[current].text;
        current++;
        return std::make_shared<StringLiteral>(val);
    }
    if (tokens[current].type == TOK_ID) {
        std::string name = tokens[current].text;
        current++;
        return std::make_shared<Identifier>(name);
    }
    if (tokens[current].type == TOK_LPAREN) {
        current++;
        std::shared_ptr<ASTNode> expr = parse_comparison(tokens, current);
        expect(tokens, current, TOK_RPAREN);
        return expr;
    }
    
    current_line = tokens[current].line;
    current_column = tokens[current].column;
    throw_parser_error("Unexpected token. Expression expected: '" + tokens[current].text + "'");
    return nullptr;
}

std::vector<std::shared_ptr<ASTNode>> parse(const std::vector<Token>& tokens) {
    std::vector<std::shared_ptr<ASTNode>> program;
    size_t current = 0;
    while (tokens[current].type != TOK_EOF) {
        program.push_back(parse_expression(tokens, current));
    }
    return program;
}

// =======================================================
// MAIN EXECUTION
// =======================================================

void runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: File '" << path << "' not found." << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    try {
        std::vector<Token> tokens = tokenize(source);
        std::vector<std::shared_ptr<ASTNode>> program = parse(tokens);
        
        Interpreter interpreter;
        load_libs(interpreter);
        interpreter.execute(program);
    }
    catch (const ULangError& e) {
        std::cerr << e.getFullMessage() << std::endl; 
    }
    catch (const std::exception& e) {
        std::cerr << "INTERNAL ERROR: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <script_file.ul>" << std::endl;
        return 1;
    }
    runFile(argv[1]);
    return 0;
}