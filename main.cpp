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
#include <iomanip>
#include <limits>
#include <functional>
#include <curl/curl.h>

#ifdef VOID
#undef VOID
#endif

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

inline void throw_lexer_error(const std::string& msg) { throw ULangError(msg, "Lexer", current_line, current_column); }
inline void throw_parser_error(const std::string& msg) { throw ULangError(msg, "Parser", current_line, current_column); }
inline void void_throw_runtime_error(const std::string& msg) { throw ULangError(msg, "Runtime", current_line, current_column); }
inline void throw_runtime_error(const std::string& msg) { throw ULangError(msg, "Runtime", current_line, current_column); }

enum TokenType {
    TOK_ID, TOK_NUMBER, TOK_STRING_LIT, TOK_LPAREN, TOK_RPAREN, TOK_COMMA, TOK_EQUALS,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT, TOK_LT, TOK_GT, TOK_EE, TOK_NE,
    TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET, TOK_SEMICOLON,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_IN, TOK_THAT, TOK_CASE,
    TOK_CLASS, TOK_THIS, TOK_DOT, TOK_NEW, TOK_FUNCTION, TOK_RETURN,
    TOK_TRY, TOK_CATCH, TOK_NULL, TOK_TRUE, TOK_FALSE, TOK_EOF
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
    if (text == "for") return TOK_FOR;
    if (text == "in") return TOK_IN;
    if (text == "that") return TOK_THAT;
    if (text == "case") return TOK_CASE;
    if (text == "class") return TOK_CLASS;
    if (text == "this") return TOK_THIS;
    if (text == "new") return TOK_NEW;
    if (text == "function") return TOK_FUNCTION;
    if (text == "return") return TOK_RETURN;
    if (text == "try") return TOK_TRY;
    if (text == "catch") return TOK_CATCH;
    if (text == "null") return TOK_NULL;
    if (text == "true") return TOK_TRUE;
    if (text == "false") return TOK_FALSE;
    return TOK_ID;
}

std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;
    current_line = 1;
    current_column = 1;
    while (i < source.length()) {
        char c = source[i];
        if (c == '\n') { i++; current_line++; current_column = 1; continue; }
        if (c == '\r') { i++; current_column++; continue; }
        if (isspace(static_cast<unsigned char>(c))) { i++; current_column++; continue; }
        int start_line = current_line;
        int start_col = current_column;
        if (c == '"') {
            i++; current_column++;
            std::string s;
            while (i < source.length()) {
                if (source[i] == '\\') {
                    i++; current_column++;
                    if (i >= source.length()) throw_lexer_error("Invalid escape sequence in string literal.");
                    char esc = source[i];
                    if (esc == 'n') s += '\n';
                    else if (esc == 't') s += '\t';
                    else if (esc == 'r') s += '\r';
                    else if (esc == '\\') s += '\\';
                    else if (esc == '"') s += '"';
                    else s += esc;
                    i++; current_column++;
                } else if (source[i] == '"') {
                    i++; current_column++;
                    break;
                } else {
                    if (source[i] == '\n') { current_line++; current_column = 1; }
                    else current_column++;
                    s += source[i++];
                }
            }
            if (i > source.length()) throw_lexer_error("Unclosed string literal.");
            tokens.push_back(Token(TOK_STRING_LIT, s, start_line, start_col));
        }
        else if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string id;
            while (i < source.length() && (isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
                id += source[i++];
            }
            tokens.push_back(Token(check_keyword(id), id, start_line, start_col));
            current_column += id.length();
        }
        else if (isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (i < source.length() && (isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.')) {
                num += source[i++];
            }
            tokens.push_back(Token(TOK_NUMBER, num, start_line, start_col));
            current_column += num.length();
        }
        else if (c == '=') {
            if (i + 1 < source.length() && source[i+1] == '=') {
                tokens.push_back(Token(TOK_EE, "==", start_line, start_col)); i+=2; current_column+=2;
            } else {
                tokens.push_back(Token(TOK_EQUALS, "=", start_line, start_col)); i++; current_column++;
            }
        }
        else if (c == '!') {
            if (i + 1 < source.length() && source[i+1] == '=') {
                tokens.push_back(Token(TOK_NE, "!=", start_line, start_col)); i+=2; current_column+=2;
            } else { i++; current_column++; throw_lexer_error("Unknown operator: '!'"); }
        }
        else if (c == '/') {
            if (i + 1 < source.length() && source[i+1] == '/') {
                i += 2; current_column += 2;
                while (i < source.length() && source[i] != '\n') { i++; current_column++; }
                continue;
            } else {
                tokens.push_back(Token(TOK_SLASH, "/", start_line, start_col)); i++; current_column++;
            }
        }
        else if (c == '-') {
            if (i + 1 < source.length() && source[i+1] == '>') {
                i += 2; current_column += 2;
                while (i < source.length() && source[i] != '\n') { i++; current_column++; }
                continue;
            } else {
                tokens.push_back(Token(TOK_MINUS, "-", start_line, start_col)); i++; current_column++;
            }
        }
        else if (c == ';') { tokens.push_back(Token(TOK_SEMICOLON, ";", start_line, start_col)); i++; current_column++; }
        else if (c == '.') { tokens.push_back(Token(TOK_DOT, ".", start_line, start_col)); i++; current_column++; }
        else if (c == '(') { tokens.push_back(Token(TOK_LPAREN, "(", start_line, start_col)); i++; current_column++; }
        else if (c == ')') { tokens.push_back(Token(TOK_RPAREN, ")", start_line, start_col)); i++; current_column++; }
        else if (c == '{') { tokens.push_back(Token(TOK_LBRACE, "{", start_line, start_col)); i++; current_column++; }
        else if (c == '}') { tokens.push_back(Token(TOK_RBRACE, "}", start_line, start_col)); i++; current_column++; }
        else if (c == '[') { tokens.push_back(Token(TOK_LBRACKET, "[", start_line, start_col)); i++; current_column++; }
        else if (c == ']') { tokens.push_back(Token(TOK_RBRACKET, "]", start_line, start_col)); i++; current_column++; }
        else if (c == ',') { tokens.push_back(Token(TOK_COMMA, ",", start_line, start_col)); i++; current_column++; }
        else if (c == '+') { tokens.push_back(Token(TOK_PLUS, "+", start_line, start_col)); i++; current_column++; }
        else if (c == '*') { tokens.push_back(Token(TOK_STAR, "*", start_line, start_col)); i++; current_column++; }
        else if (c == '%') { tokens.push_back(Token(TOK_PERCENT, "%", start_line, start_col)); i++; current_column++; }
        else if (c == '<') { tokens.push_back(Token(TOK_LT, "<", start_line, start_col)); i++; current_column++; }
        else if (c == '>') { tokens.push_back(Token(TOK_GT, ">", start_line, start_col)); i++; current_column++; }
        else { i++; current_column++; throw_lexer_error("Unknown character"); }
    }
    tokens.push_back(Token(TOK_EOF, "", current_line, current_column));
    return tokens;
}

class Interpreter;
class ULangObject;
class ASTNode;
class InstanceObject;

class ULangObject : public std::enable_shared_from_this<ULangObject> {
public:
    enum Type { NUMBER, STRING, BOOLEAN, FUNCTION, VOID_TYPE, CLASS, INSTANCE, LIST, BUILTIN };
    Type type;
    ULangObject(Type t) : type(t) {}
    virtual ~ULangObject() = default;
    virtual std::string toString() const = 0;
    virtual double toDouble() const { return 0.0; }
    virtual bool isTruthy() const { return type != VOID_TYPE && type != BOOLEAN ? true : (type == BOOLEAN ? toDouble() : false); }
    virtual std::shared_ptr<ULangObject> getMethod(const std::string& name) { return nullptr; }
};

class VoidObject : public ULangObject {
public:
    VoidObject() : ULangObject(VOID_TYPE) {}
    std::string toString() const override { return "null"; }
    bool isTruthy() const override { return false; }
};
static std::shared_ptr<ULangObject> VOID_INSTANCE = std::make_shared<VoidObject>();

class NumberObject : public ULangObject {
public:
    double value;
    NumberObject(double v) : ULangObject(NUMBER), value(v) {}
    std::string toString() const override {
        std::stringstream ss;
        ss << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
        return ss.str();
    }
    double toDouble() const override { return value; }
    bool isTruthy() const override { return value != 0.0; }
};

class StringObject : public ULangObject {
public:
    std::string value;
    StringObject(const std::string& v) : ULangObject(STRING), value(v) {}
    std::string toString() const override { return value; }
    bool isTruthy() const override { return !value.empty(); }
};

class BooleanObject : public ULangObject {
public:
    bool value;
    BooleanObject(bool v) : ULangObject(BOOLEAN), value(v) {}
    std::string toString() const override { return value ? "true" : "false"; }
    double toDouble() const override { return value ? 1.0 : 0.0; }
    bool isTruthy() const override { return value; }
};

class FunctionObject : public ULangObject {
public:
    std::vector<std::string> params;
    std::vector<std::shared_ptr<ASTNode>> body;
    std::shared_ptr<InstanceObject> receiver;
    FunctionObject(const std::vector<std::string>& p, const std::vector<std::shared_ptr<ASTNode>>& b, std::shared_ptr<InstanceObject> r = nullptr)
        : ULangObject(FUNCTION), params(p), body(b), receiver(r) {}
    std::string toString() const override { return "<function>"; }
    std::shared_ptr<FunctionObject> bind(std::shared_ptr<InstanceObject> instance) {
        return std::make_shared<FunctionObject>(params, body, instance);
    }
    virtual std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args);
};

class BuiltinFunction : public ULangObject {
public:
    using FuncType = std::function<std::shared_ptr<ULangObject>(Interpreter&, const std::vector<std::shared_ptr<ULangObject>>&)> ;
    FuncType func;
    std::string name;
    BuiltinFunction(const std::string& n, FuncType f) : ULangObject(BUILTIN), func(f), name(n) {}
    std::string toString() const override { return "<builtin " + name + ">"; }
    std::shared_ptr<ULangObject> call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) { return func(interpreter, args); }
};

class ListObject : public ULangObject {
public:
    std::vector<std::shared_ptr<ULangObject>> elements;
    ListObject(const std::vector<std::shared_ptr<ULangObject>>& e) : ULangObject(LIST), elements(e) {}
    std::string toString() const override {
        std::string s = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            s += elements[i]->toString();
            if (i < elements.size() - 1) s += ", ";
        }
        return s + "]";
    }
    std::shared_ptr<ULangObject> getMethod(const std::string& name) override;
};

class ClassObject : public ULangObject {
public:
    std::string name;
    std::map<std::string, std::shared_ptr<FunctionObject>> methods;
    ClassObject(const std::string& n, const std::map<std::string, std::shared_ptr<FunctionObject>>& m)
        : ULangObject(CLASS), name(n), methods(m) {}
    std::string toString() const override { return "<class " + name + ">"; }
};

class InstanceObject : public ULangObject {
public:
    std::shared_ptr<ClassObject> klass;
    std::map<std::string, std::shared_ptr<ULangObject>> fields;
    InstanceObject(std::shared_ptr<ClassObject> k) : ULangObject(INSTANCE), klass(k) {}
    std::string toString() const override { return "<instance of " + klass->name + ">"; }
    std::shared_ptr<ULangObject> getProperty(const std::string& name) {
        if (fields.count(name)) return fields.at(name);
        if (klass->methods.count(name)) {
            return klass->methods.at(name)->bind(std::static_pointer_cast<InstanceObject>(shared_from_this()));
        }
        throw_runtime_error("Undefined property '" + name + "'.");
        return VOID_INSTANCE;
    }
    void setProperty(const std::string& name, std::shared_ptr<ULangObject> value) {
        fields[name] = value;
    }
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) = 0;
};

class Interpreter {
public:
    std::map<std::string, std::shared_ptr<ULangObject>> globals;
    std::map<std::string, std::shared_ptr<ULangObject>>* current_env;
    std::vector<std::map<std::string, std::shared_ptr<ULangObject>>> env_stack;
    std::shared_ptr<InstanceObject> current_instance = nullptr;

    Interpreter() {
        env_stack.push_back(std::map<std::string, std::shared_ptr<ULangObject>>());
        current_env = &env_stack.back();
        loadLibs();
    }

    void define(const std::string& name, std::shared_ptr<ULangObject> val) {
        (*current_env)[name] = val;
    }

    void assign(const std::string& name, std::shared_ptr<ULangObject> val) {
        for (auto it = env_stack.rbegin(); it != env_stack.rend(); ++it) {
            if (it->count(name)) {
                (*it)[name] = val;
                return;
            }
        }
        throw_runtime_error("Undefined variable '" + name + "'.");
    }

    std::shared_ptr<ULangObject> lookup(const std::string& name) {
        for (auto it = env_stack.rbegin(); it != env_stack.rend(); ++it) {
            if (it->count(name)) return (*it)[name];
        }
        throw_runtime_error("Undefined variable '" + name + "'.");
        return VOID_INSTANCE;
    }

    void pushEnv() {
        env_stack.push_back(std::map<std::string, std::shared_ptr<ULangObject>>());
        current_env = &env_stack.back();
    }

    void popEnv() {
        if (env_stack.size() > 1) {
            env_stack.pop_back();
            current_env = &env_stack.back();
        }
    }

    void enterInstanceContext(std::shared_ptr<InstanceObject> instance) { current_instance = instance; }
    void exitInstanceContext() { current_instance = nullptr; }
    std::shared_ptr<InstanceObject> getCurrentInstance() { return current_instance; }

    void loadLibs();
    std::shared_ptr<ULangObject> executeBlock(const std::vector<std::shared_ptr<ASTNode>>& statements);
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

std::shared_ptr<ULangObject> ListObject::getMethod(const std::string& name) {
    if (name == "append") {
        return std::make_shared<BuiltinFunction>("append", [this](Interpreter& i, const std::vector<std::shared_ptr<ULangObject>>& args) {
            if (args.size() != 1) throw_runtime_error("append expects 1 argument.");
            this->elements.push_back(args[0]);
            return VOID_INSTANCE;
        });
    }
    if (name == "pop") {
        return std::make_shared<BuiltinFunction>("pop", [this](Interpreter& i, const std::vector<std::shared_ptr<ULangObject>>& args) {
            if (this->elements.empty()) throw_runtime_error("Pop from empty list.");
            auto val = this->elements.back();
            this->elements.pop_back();
            return val;
        });
    }
    return nullptr;
}

std::shared_ptr<ULangObject> FunctionObject::call(Interpreter& interpreter, const std::vector<std::shared_ptr<ULangObject>>& args) {
    interpreter.pushEnv();
    if (receiver) interpreter.enterInstanceContext(receiver);

    for (size_t i = 0; i < params.size(); ++i) {
        if (i < args.size()) interpreter.define(params[i], args[i]);
    }

    std::shared_ptr<ULangObject> result = VOID_INSTANCE;
    try {
        result = interpreter.executeBlock(body);
    } catch (std::shared_ptr<ULangObject> retVal) {
        result = retVal;
    } catch (...) {
        if (receiver) interpreter.exitInstanceContext();
        interpreter.popEnv();
        throw;
    }

    if (receiver) interpreter.exitInstanceContext();
    interpreter.popEnv();
    return result;
}

std::shared_ptr<ULangObject> Interpreter::executeBlock(const std::vector<std::shared_ptr<ASTNode>>& statements) {
    std::shared_ptr<ULangObject> result = VOID_INSTANCE;
    for (const auto& stmt : statements) {
        result = stmt->evaluate(*this);
    }
    return result;
}

void Interpreter::loadLibs() {
    define("output", std::make_shared<BuiltinFunction>("output", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) {
        for (auto& arg : args) std::cout << arg->toString() << " ";
        std::cout << std::endl;
        return VOID_INSTANCE;
    }));
    define("len", std::make_shared<BuiltinFunction>("len", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) {
        if (args.size() != 1) throw_runtime_error("len expects 1 argument");
        if (args[0]->type == ULangObject::LIST) return std::make_shared<NumberObject>((double)std::static_pointer_cast<ListObject>(args[0])->elements.size());
        if (args[0]->type == ULangObject::STRING) return std::make_shared<NumberObject>((double)std::static_pointer_cast<StringObject>(args[0])->value.length());
        return std::make_shared<NumberObject>(0.0);
    }));
    define("floor", std::make_shared<BuiltinFunction>("floor", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) {
        if (args.empty() || args[0]->type != ULangObject::NUMBER) throw_runtime_error("floor expects number");
        return std::make_shared<NumberObject>(std::floor(std::static_pointer_cast<NumberObject>(args[0])->value));
    }));
    define("pow", std::make_shared<BuiltinFunction>("pow", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) {
        if (args.size() != 2) throw_runtime_error("pow expects 2 arguments");
        return std::make_shared<NumberObject>(std::pow(args[0]->toDouble(), args[1]->toDouble()));
    }));
    define("drawGraph", std::make_shared<BuiltinFunction>("drawGraph", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) {
        if (args.empty() || args[0]->type != ULangObject::LIST) throw_runtime_error("drawGraph expects a list");
        auto list = std::static_pointer_cast<ListObject>(args[0]);
        std::cout << "\n--- GRAPH ---\n";
        for (auto& item : list->elements) {
            if (item->type == ULangObject::NUMBER) {
                int val = (int)item->toDouble();
                std::cout << val << " | ";
                for(int k=0; k<val; ++k) std::cout << "*";
                std::cout << "\n";
            }
        }
        return VOID_INSTANCE;
    }));
    define("http_post", std::make_shared<BuiltinFunction>("http_post", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) -> std::shared_ptr<ULangObject> {
        if (args.size() != 3 || args[0]->type != ULangObject::STRING || args[1]->type != ULangObject::STRING || args[2]->type != ULangObject::LIST)
            throw_runtime_error("http_post expects 3 arguments: URL (string), BODY (string), HEADERS (list)");

        std::string url = std::static_pointer_cast<StringObject>(args[0])->value;
        std::string body = std::static_pointer_cast<StringObject>(args[1])->value;
        auto headerList = std::static_pointer_cast<ListObject>(args[2]);

        std::string response_buffer;
        CURL* curl = curl_easy_init();
        struct curl_slist* headers = nullptr;

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

            for (auto& headerObj : headerList->elements) {
                if (headerObj->type == ULangObject::STRING) {
                    headers = curl_slist_append(headers, std::static_pointer_cast<StringObject>(headerObj)->value.c_str());
                }
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) throw_runtime_error("http_post failed: " + std::string(curl_easy_strerror(res)));
            return std::make_shared<StringObject>(response_buffer);
        }
        throw_runtime_error("Failed to initialize cURL");
        return VOID_INSTANCE;
    }));
    define("http_get", std::make_shared<BuiltinFunction>("http_get", [](Interpreter&, const std::vector<std::shared_ptr<ULangObject>>& args) -> std::shared_ptr<ULangObject> {
        if (args.size() != 1 || args[0]->type != ULangObject::STRING) throw_runtime_error("http_get expects 1 string argument (URL)");

        std::string url = std::static_pointer_cast<StringObject>(args[0])->value;
        std::string response_buffer;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) throw_runtime_error("http_get failed: " + std::string(curl_easy_strerror(res)));
            return std::make_shared<StringObject>(response_buffer);
        }
        throw_runtime_error("Failed to initialize cURL");
        return VOID_INSTANCE;
    }));
}

// ... (kalan AST node sınıfları aynı kalacak, sadece VOID yerine VOID_TYPE kullan)

class NumberNode : public ASTNode {
    double value;
public:
    NumberNode(double v) : value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        return std::make_shared<NumberObject>(value);
    }
};

class StringNode : public ASTNode {
    std::string value;
public:
    StringNode(std::string v) : value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        return std::make_shared<StringObject>(value);
    }
};

class VariableNode : public ASTNode {
public:
    std::string name;
    VariableNode(std::string n) : name(n) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        return interpreter.lookup(name);
    }
};

class BinaryOpNode : public ASTNode {
    std::string op;
    std::shared_ptr<ASTNode> left, right;
public:
    BinaryOpNode(std::string o, std::shared_ptr<ASTNode> l, std::shared_ptr<ASTNode> r) : op(o), left(l), right(r) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto l = left->evaluate(interpreter);
        auto r = right->evaluate(interpreter);
        if (op == "==") return std::make_shared<BooleanObject>(l->toString() == r->toString());
        if (op == "!=") return std::make_shared<BooleanObject>(l->toString() != r->toString());

        if (l->type == ULangObject::NUMBER && r->type == ULangObject::NUMBER) {
            double v1 = l->toDouble();
            double v2 = r->toDouble();
            if (op == "+") return std::make_shared<NumberObject>(v1 + v2);
            if (op == "-") return std::make_shared<NumberObject>(v1 - v2);
            if (op == "*") return std::make_shared<NumberObject>(v1 * v2);
            if (op == "/") return std::make_shared<NumberObject>(v1 / v2);
            if (op == "%") return std::make_shared<NumberObject>(std::fmod(v1, v2));
            if (op == "<") return std::make_shared<BooleanObject>(v1 < v2);
            if (op == ">") return std::make_shared<BooleanObject>(v1 > v2);
        }
        if (op == "+") return std::make_shared<StringObject>(l->toString() + r->toString());
        throw_runtime_error("Invalid binary operation");
        return VOID_INSTANCE;
    }
};

class AssignmentNode : public ASTNode {
    std::string name;
    std::shared_ptr<ASTNode> value;
public:
    AssignmentNode(std::string n, std::shared_ptr<ASTNode> v) : name(n), value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto res = value->evaluate(interpreter);
        try {
            interpreter.assign(name, res);
        } catch(...) {
            interpreter.define(name, res);
        }
        return res;
    }
};

class VarDeclNode : public ASTNode {
    std::string name;
    std::shared_ptr<ASTNode> value;
public:
    VarDeclNode(std::string n, std::shared_ptr<ASTNode> v) : name(n), value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto res = value->evaluate(interpreter);
        interpreter.define(name, res);
        return res;
    }
};

class BlockNode : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> statements;
    BlockNode(std::vector<std::shared_ptr<ASTNode>> s) : statements(s) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        return interpreter.executeBlock(statements);
    }
};

class IfNode : public ASTNode {
    std::shared_ptr<ASTNode> condition, thenBlock, elseBlock;
public:
    IfNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> t, std::shared_ptr<ASTNode> e) : condition(c), thenBlock(t), elseBlock(e) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        if (condition->evaluate(interpreter)->isTruthy()) return thenBlock->evaluate(interpreter);
        else if (elseBlock) return elseBlock->evaluate(interpreter);
        return VOID_INSTANCE;
    }
};

class WhileNode : public ASTNode {
    std::shared_ptr<ASTNode> condition, body;
public:
    WhileNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> b) : condition(c), body(b) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        while (condition->evaluate(interpreter)->isTruthy()) {
            body->evaluate(interpreter);
        }
        return VOID_INSTANCE;
    }
};

class ForNode : public ASTNode {
    std::string varName;
    std::shared_ptr<ASTNode> iterator, body;
public:
    ForNode(std::string v, std::shared_ptr<ASTNode> i, std::shared_ptr<ASTNode> b) : varName(v), iterator(i), body(b) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto listObj = iterator->evaluate(interpreter);
        if (listObj->type != ULangObject::LIST) throw_runtime_error("For loop expects list");
        auto list = std::static_pointer_cast<ListObject>(listObj);
        interpreter.pushEnv();
        interpreter.define(varName, VOID_INSTANCE);
        for (auto& elem : list->elements) {
            interpreter.assign(varName, elem);
            body->evaluate(interpreter);
        }
        interpreter.popEnv();
        return VOID_INSTANCE;
    }
};

class CallNode : public ASTNode {
    std::shared_ptr<ASTNode> callee;
    std::vector<std::shared_ptr<ASTNode>> args;
public:
    CallNode(std::shared_ptr<ASTNode> c, std::vector<std::shared_ptr<ASTNode>> a) : callee(c), args(a) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto func = callee->evaluate(interpreter);
        std::vector<std::shared_ptr<ULangObject>> evalArgs;
        for(auto& a : args) evalArgs.push_back(a->evaluate(interpreter));

        if (func->type == ULangObject::FUNCTION) return std::static_pointer_cast<FunctionObject>(func)->call(interpreter, evalArgs);
        if (func->type == ULangObject::BUILTIN) return std::static_pointer_cast<BuiltinFunction>(func)->call(interpreter, evalArgs);
        if (func->type == ULangObject::CLASS) {
            auto klass = std::static_pointer_cast<ClassObject>(func);
            auto instance = std::make_shared<InstanceObject>(klass);
            if (klass->methods.count("__init__")) {
                auto init = klass->methods.at("__init__")->bind(instance);
                init->call(interpreter, evalArgs);
            }
            return instance;
        }
        throw_runtime_error("Not callable");
        return VOID_INSTANCE;
    }
};

class InstanceCreation : public ASTNode {
    std::string className;
    std::vector<std::shared_ptr<ASTNode>> args;
public:
    InstanceCreation(std::string c, std::vector<std::shared_ptr<ASTNode>> a) : className(c), args(a) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto classObj = interpreter.lookup(className);
        if (classObj->type != ULangObject::CLASS) throw_runtime_error("Not a class");
        auto klass = std::static_pointer_cast<ClassObject>(classObj);
        auto instance = std::make_shared<InstanceObject>(klass);
        std::vector<std::shared_ptr<ULangObject>> evalArgs;
        for(auto& a : args) evalArgs.push_back(a->evaluate(interpreter));
        if (klass->methods.count("__init__")) {
            auto init = klass->methods.at("__init__")->bind(instance);
            init->call(interpreter, evalArgs);
        }
        return instance;
    }
};

class ReturnNode : public ASTNode {
    std::shared_ptr<ASTNode> value;
public:
    ReturnNode(std::shared_ptr<ASTNode> v) : value(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto res = value ? value->evaluate(interpreter) : VOID_INSTANCE;
        throw res;
    }
};

class FunctionDeclNode : public ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<ASTNode> body;
public:
    FunctionDeclNode(std::string n, std::vector<std::string> p, std::shared_ptr<ASTNode> b) : name(n), params(p), body(b) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        std::vector<std::shared_ptr<ASTNode>> stmts;
        if (auto b = std::dynamic_pointer_cast<BlockNode>(body)) stmts = b->statements;
        else stmts.push_back(body);

        auto func = std::make_shared<FunctionObject>(params, stmts);
        interpreter.define(name, func);
        return func;
    }
};

class ClassNode : public ASTNode {
    std::string name;
    std::map<std::string, std::shared_ptr<FunctionObject>> methods;
public:
    ClassNode(std::string n, std::map<std::string, std::shared_ptr<FunctionObject>> m) : name(n), methods(m) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        interpreter.define(name, std::make_shared<ClassObject>(name, methods));
        return VOID_INSTANCE;
    }
};

class PropertyGetNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> obj;
    std::string prop;
    PropertyGetNode(std::shared_ptr<ASTNode> o, std::string p) : obj(o), prop(p) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto o = obj->evaluate(interpreter);
        if(o->type == ULangObject::INSTANCE) return std::static_pointer_cast<InstanceObject>(o)->getProperty(prop);
        if(o->type == ULangObject::LIST) return std::static_pointer_cast<ListObject>(o)->getMethod(prop);
        throw_runtime_error("Property access on invalid object");
        return VOID_INSTANCE;
    }
};

class PropertySetNode : public ASTNode {
    std::shared_ptr<ASTNode> obj;
    std::string prop;
    std::shared_ptr<ASTNode> val;
public:
    PropertySetNode(std::shared_ptr<ASTNode> o, std::string p, std::shared_ptr<ASTNode> v) : obj(o), prop(p), val(v) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        auto o = obj->evaluate(interpreter);
        auto v = val->evaluate(interpreter);
        if(o->type == ULangObject::INSTANCE) {
            std::static_pointer_cast<InstanceObject>(o)->setProperty(prop, v);
            return v;
        }
        throw_runtime_error("Property set on invalid object");
        return VOID_INSTANCE;
    }
};

class ListNode : public ASTNode {
    std::vector<std::shared_ptr<ASTNode>> elements;
public:
    ListNode(std::vector<std::shared_ptr<ASTNode>> e) : elements(e) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        std::vector<std::shared_ptr<ULangObject>> values;
        for(auto& e : elements) values.push_back(e->evaluate(interpreter));
        return std::make_shared<ListObject>(values);
    }
};

class TryCatchNode : public ASTNode {
    std::shared_ptr<ASTNode> tryBlock;
    std::string catchVar;
    std::shared_ptr<ASTNode> catchBlock;
public:
    TryCatchNode(std::shared_ptr<ASTNode> t, std::string v, std::shared_ptr<ASTNode> c) : tryBlock(t), catchVar(v), catchBlock(c) {}
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        try {
            return tryBlock->evaluate(interpreter);
        } catch (const std::exception& e) {
            interpreter.pushEnv();
            interpreter.define(catchVar, std::make_shared<StringObject>(e.what()));
            auto res = catchBlock->evaluate(interpreter);
            interpreter.popEnv();
            return res;
        } catch (std::shared_ptr<ULangObject> e) {
             interpreter.pushEnv();
             interpreter.define(catchVar, e);
             auto res = catchBlock->evaluate(interpreter);
             interpreter.popEnv();
             return res;
        }
    }
};

class ThisNode : public ASTNode {
public:
    std::shared_ptr<ULangObject> evaluate(Interpreter& interpreter) override {
        if (interpreter.current_instance) return interpreter.current_instance;
        throw_runtime_error("this used outside of instance");
        return VOID_INSTANCE;
    }
};

class Parser {
    std::vector<Token> tokens;
    int pos = 0;
public:
    Parser(std::vector<Token> t) : tokens(t) {}

    Token peek() { return tokens[pos]; }
    bool isAtEnd() { return peek().type == TOK_EOF; }
    Token advance() { if (!isAtEnd()) pos++; return tokens[pos-1]; }
    bool check(TokenType t) { return !isAtEnd() && peek().type == t; }
    Token consume(TokenType t, std::string msg) { if (check(t)) return advance(); throw_parser_error(msg); return tokens[0]; }

    std::vector<std::shared_ptr<ASTNode>> parse() {
        std::vector<std::shared_ptr<ASTNode>> stmts;
        while (!isAtEnd()) stmts.push_back(declaration());
        return stmts;
    }

    std::shared_ptr<ASTNode> declaration() {
        if (check(TOK_FUNCTION)) return functionDecl();
        if (check(TOK_CLASS)) return classDecl();
        return statement();
    }

    std::shared_ptr<ASTNode> classDecl() {
        consume(TOK_CLASS, "Expect class");
        std::string name = consume(TOK_ID, "Expect class name").text;
        consume(TOK_LBRACE, "Expect {");
        std::map<std::string, std::shared_ptr<FunctionObject>> methods;
        while(!check(TOK_RBRACE) && !isAtEnd()) {
            std::string mName = consume(TOK_ID, "Expect method name").text;
            consume(TOK_LPAREN, "Expect (");
            std::vector<std::string> params;
            if (!check(TOK_RPAREN)) {
                do { params.push_back(consume(TOK_ID, "Param name").text); } while(check(TOK_COMMA) && advance().type == TOK_COMMA);
            }
            consume(TOK_RPAREN, "Expect )");
            consume(TOK_LBRACE, "Expect {");
            auto body = block();
            std::vector<std::shared_ptr<ASTNode>> bodyStmts;
            if (auto b = std::dynamic_pointer_cast<BlockNode>(body)) {
                bodyStmts = b->statements;
            } else {
                bodyStmts.push_back(body);
            }
            methods[mName] = std::make_shared<FunctionObject>(params, bodyStmts);
        }
        consume(TOK_RBRACE, "Expect }");
        return std::make_shared<ClassNode>(name, methods);
    }

    std::shared_ptr<ASTNode> functionDecl() {
        consume(TOK_FUNCTION, "Expect function");
        std::string name = consume(TOK_ID, "Expect name").text;
        consume(TOK_LPAREN, "Expect (");
        std::vector<std::string> params;
        if (!check(TOK_RPAREN)) {
            do { params.push_back(consume(TOK_ID, "Param").text); } while(check(TOK_COMMA) && advance().type == TOK_COMMA);
        }
        consume(TOK_RPAREN, "Expect )");
        consume(TOK_LBRACE, "Expect {");
        return std::make_shared<FunctionDeclNode>(name, params, block());
    }

    std::shared_ptr<ASTNode> statement() {
        if (check(TOK_IF)) return ifStmt();
        if (check(TOK_WHILE)) return whileStmt();
        if (check(TOK_FOR)) return forStmt();
        if (check(TOK_RETURN)) return returnStmt();
        if (check(TOK_TRY)) return tryStmt();
        if (check(TOK_LBRACE)) { consume(TOK_LBRACE, "{"); return block(); }
        auto expr = expression();
        if (check(TOK_SEMICOLON)) advance();
        return expr;
    }

    std::shared_ptr<ASTNode> ifStmt() {
        consume(TOK_IF, "Expect if");
        consume(TOK_LPAREN, "Expect (");
        auto cond = expression();
        consume(TOK_RPAREN, "Expect )");
        if (check(TOK_IN)) { advance(); consume(TOK_THAT, "that"); consume(TOK_CASE, "case"); }
        consume(TOK_LBRACE, "Expect {");
        auto thenB = block();
        std::shared_ptr<ASTNode> elseB = nullptr;
        if (check(TOK_ELSE)) {
            advance();
            if (check(TOK_IF)) elseB = ifStmt();
            else { consume(TOK_LBRACE, "{"); elseB = block(); }
        }
        return std::make_shared<IfNode>(cond, thenB, elseB);
    }

    std::shared_ptr<ASTNode> whileStmt() {
        consume(TOK_WHILE, "Expect while");
        consume(TOK_LPAREN, "Expect (");
        auto cond = expression();
        consume(TOK_RPAREN, "Expect )");
        consume(TOK_LBRACE, "Expect {");
        return std::make_shared<WhileNode>(cond, block());
    }

    std::shared_ptr<ASTNode> forStmt() {
        consume(TOK_FOR, "Expect for");
        std::string var = consume(TOK_ID, "Expect var").text;
        consume(TOK_IN, "Expect in");
        auto iter = expression();
        consume(TOK_LBRACE, "Expect {");
        return std::make_shared<ForNode>(var, iter, block());
    }

    std::shared_ptr<ASTNode> returnStmt() {
        consume(TOK_RETURN, "Expect return");
        std::shared_ptr<ASTNode> val = nullptr;
        if (peek().type != TOK_RBRACE && peek().type != TOK_EOF) val = expression();
        if (check(TOK_SEMICOLON)) advance();
        return std::make_shared<ReturnNode>(val);
    }

    std::shared_ptr<ASTNode> tryStmt() {
        consume(TOK_TRY, "Expect try");
        consume(TOK_LBRACE, "Expect {");
        auto tryB = block();
        consume(TOK_CATCH, "Expect catch");
        consume(TOK_LPAREN, "Expect (");
        std::string v = consume(TOK_ID, "Expect var").text;
        consume(TOK_RPAREN, "Expect )");
        consume(TOK_LBRACE, "Expect {");
        auto catchB = block();
        return std::make_shared<TryCatchNode>(tryB, v, catchB);
    }

    std::shared_ptr<ASTNode> block() {
        std::vector<std::shared_ptr<ASTNode>> stmts;
        while (!check(TOK_RBRACE) && !isAtEnd()) {
            stmts.push_back(declaration());
        }
        consume(TOK_RBRACE, "Expect }");
        return std::make_shared<BlockNode>(stmts);
    }

    std::shared_ptr<ASTNode> expression() { return assignment(); }

    std::shared_ptr<ASTNode> assignment() {
        auto expr = equality();
        if (check(TOK_EQUALS)) {
            advance();
            auto val = assignment();
            if (auto v = std::dynamic_pointer_cast<VariableNode>(expr)) return std::make_shared<AssignmentNode>(v->name, val);
            if (auto p = std::dynamic_pointer_cast<PropertyGetNode>(expr)) return std::make_shared<PropertySetNode>(p->obj, p->prop, val);
        }
        return expr;
    }

    std::shared_ptr<ASTNode> equality() {
        auto expr = comparison();
        while (check(TOK_EE) || check(TOK_NE)) {
            std::string op = advance().text;
            expr = std::make_shared<BinaryOpNode>(op, expr, comparison());
        }
        return expr;
    }

    std::shared_ptr<ASTNode> comparison() {
        auto expr = term();
        while (check(TOK_LT) || check(TOK_GT)) {
            std::string op = advance().text;
            expr = std::make_shared<BinaryOpNode>(op, expr, term());
        }
        return expr;
    }

    std::shared_ptr<ASTNode> term() {
        auto expr = factor();
        while (check(TOK_PLUS) || check(TOK_MINUS)) {
            std::string op = advance().text;
            expr = std::make_shared<BinaryOpNode>(op, expr, factor());
        }
        return expr;
    }

    std::shared_ptr<ASTNode> factor() {
        auto expr = unary();
        while (check(TOK_STAR) || check(TOK_SLASH) || check(TOK_PERCENT)) {
            std::string op = advance().text;
            expr = std::make_shared<BinaryOpNode>(op, expr, unary());
        }
        return expr;
    }

    std::shared_ptr<ASTNode> unary() { return call(); }

    std::shared_ptr<ASTNode> call() {
        auto expr = primary();
        while (true) {
            if (check(TOK_LPAREN)) {
                advance();
                std::vector<std::shared_ptr<ASTNode>> args;
                if (!check(TOK_RPAREN)) {
                    do { args.push_back(expression()); } while(check(TOK_COMMA) && advance().type == TOK_COMMA);
                }
                consume(TOK_RPAREN, "Expect )");
                expr = std::make_shared<CallNode>(expr, args);
            } else if (check(TOK_DOT)) {
                advance();
                std::string prop = consume(TOK_ID, "Expect property").text;
                expr = std::make_shared<PropertyGetNode>(expr, prop);
            } else {
                break;
            }
        }
        return expr;
    }

    std::shared_ptr<ASTNode> primary() {
        if (check(TOK_NEW)) {
            advance();
            std::string className = consume(TOK_ID, "Expect class name").text;
            consume(TOK_LPAREN, "Expect (");
            std::vector<std::shared_ptr<ASTNode>> args;
            if (!check(TOK_RPAREN)) {
                do { args.push_back(expression()); } while(check(TOK_COMMA) && advance().type == TOK_COMMA);
            }
            consume(TOK_RPAREN, "Expect )");
            return std::make_shared<InstanceCreation>(className, args);
        }
        if (check(TOK_FALSE)) { advance(); return std::make_shared<NumberNode>(0); }
        if (check(TOK_TRUE)) { advance(); return std::make_shared<NumberNode>(1); }
        if (check(TOK_NULL)) { advance(); return std::make_shared<StringNode>("null"); }
        if (check(TOK_THIS)) { advance(); return std::make_shared<ThisNode>(); }
        if (check(TOK_NUMBER)) return std::make_shared<NumberNode>(std::stod(advance().text));
        if (check(TOK_STRING_LIT)) return std::make_shared<StringNode>(advance().text);
        if (check(TOK_ID)) {
            return std::make_shared<VariableNode>(advance().text);
        }
        if (check(TOK_LPAREN)) {
            advance();
            auto expr = expression();
            consume(TOK_RPAREN, "Expect )");
            return expr;
        }
        if (check(TOK_LBRACKET)) {
            advance();
            std::vector<std::shared_ptr<ASTNode>> elems;
            if (!check(TOK_RBRACKET)) {
                do { elems.push_back(expression()); } while(check(TOK_COMMA) && advance().type == TOK_COMMA);
            }
            consume(TOK_RBRACKET, "Expect ]");
            return std::make_shared<ListNode>(elems);
        }
        throw_parser_error("Expect expression");
        return nullptr;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: ulang file.ul\n"; return 1; }
    std::ifstream f(argv[1]);
    std::stringstream buff;
    buff << f.rdbuf();
    try {
        auto tokens = tokenize(buff.str());
        Parser parser(tokens);
        auto nodes = parser.parse();
        Interpreter interpreter;
        interpreter.executeBlock(nodes);
    } catch (ULangError& e) {
        std::cerr << e.getFullMessage() << "\n";
    } catch (std::exception& e) {
        std::cerr << "INTERNAL ERROR: " << e.what() << "\n";
    }
    return 0;
}
