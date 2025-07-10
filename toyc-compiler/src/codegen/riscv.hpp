#pragma once
#include "ast/ast.hpp"
#include "semantic/analyzer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

// 寄存器管理
class RegisterManager {
private:
    std::vector<bool> used;  // t0-t6, s0-s11
    static const std::vector<std::string> tempRegs;
    static const std::vector<std::string> savedRegs;
    
public:
    RegisterManager();
    
    std::string allocateTemp();
    std::string allocateSaved();
    void releaseRegister(const std::string& reg);
    void releaseAllTemp();
    bool isRegisterUsed(const std::string& reg) const;
    
private:
    int getRegisterIndex(const std::string& reg) const;
};

// 代码生成器
class RISCVCodeGenerator : public Visitor {
private:
    std::ostringstream output;
    RegisterManager regManager;
    std::unordered_map<std::string, Symbol> symbolTable;
    std::unordered_map<std::string, FunctionInfo> functionTable;
    int labelCounter;
    int currentFrameSize;
    std::string currentFunction;
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    
public:
    RISCVCodeGenerator() : labelCounter(0), currentFrameSize(0) {}
    
    std::string generate(CompilationUnit& unit, const std::unordered_map<std::string, FunctionInfo>& functions);
    
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
    std::string newLabel(const std::string& prefix = "L");
    void emit(const std::string& instruction);
    void emitLabel(const std::string& label);
    void emitComment(const std::string& comment);
    
    // 辅助函数
    std::string loadImmediate(int value, const std::string& reg);
    std::string getVariableAddress(const std::string& varName);
    void generateFunctionPrologue(const std::string& funcName, int frameSize);
    void generateFunctionEpilogue();
    void saveRegisters(const std::vector<std::string>& regs);
    void restoreRegisters(const std::vector<std::string>& regs);
    
    // 表达式求值，返回包含结果的寄存器
    std::string evaluateExpression(Expression& expr);
    
    // 计算栈帧大小
    int calculateFrameSize(const std::vector<Parameter>& params, Block& body);
    void collectLocalVariables(Block& body, std::unordered_map<std::string, int>& locals, int& offset);
};