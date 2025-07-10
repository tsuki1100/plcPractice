#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
#include <vector>
#include <string>

// 符号表项
struct Symbol {
    std::string name;
    Expression::Type type;
    int offset;
    bool isParameter;
    
    Symbol(const std::string& n, Expression::Type t, int off = 0, bool param = false)
        : name(n), type(t), offset(off), isParameter(param) {}
};

// 函数信息
struct FunctionInfo {
    std::string name;
    Expression::Type returnType;
    std::vector<Expression::Type> paramTypes;
    bool isDefined;
    
    FunctionInfo(const std::string& n, Expression::Type ret, 
                const std::vector<Expression::Type>& params, bool def = false)
        : name(n), returnType(ret), paramTypes(params), isDefined(def) {}
};

// 作用域管理
class Scope {
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopeStack;
    int currentOffset;
    
public:
    Scope() : currentOffset(0) {
        enterScope(); // 全局作用域
    }
    
    void enterScope() {
        scopeStack.push_back(std::unordered_map<std::string, Symbol>());
    }
    
    void exitScope() {
        if (scopeStack.size() > 1) {
            scopeStack.pop_back();
        }
    }
    
    bool declareVariable(const std::string& name, Expression::Type type, bool isParam = false) {
        auto& currentScope = scopeStack.back();
        if (currentScope.find(name) != currentScope.end()) {
            return false; // 重复声明
        }
        
        int offset = isParam ? currentOffset : (currentOffset - 4);
        currentScope[name] = Symbol(name, type, offset, isParam);
        if (!isParam) currentOffset -= 4;
        return true;
    }
    
    Symbol* lookupVariable(const std::string& name) {
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
    
    void resetOffset() { currentOffset = 0; }
};

// 简化的语义分析器
class SemanticAnalyzer : public Visitor {
private:
    Scope scope;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::vector<std::string> errors;
    std::string currentFunction;
    int loopDepth;
    bool hasReturn;
    
public:
    SemanticAnalyzer() : loopDepth(0), hasReturn(false) {}
    
    bool analyze(CompilationUnit& unit);
    const std::vector<std::string>& getErrors() const { return errors; }
    
    // Visitor接口实现
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(NumberLiteral& node) override;
    void visit(Identifier& node) override;
    void visit(FunctionCall& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(Block& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(FunctionDefinition& node) override;
    void visit(CompilationUnit& node) override;
    
private:
    void addError(const std::string& message);
    bool checkMainFunction();
};