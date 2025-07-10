#include "codegen/riscv.hpp"
#include <iostream>
#include <algorithm>

// RegisterManager实现
const std::vector<std::string> RegisterManager::tempRegs = {
    "t0", "t1", "t2", "t3", "t4", "t5", "t6"
};

const std::vector<std::string> RegisterManager::savedRegs = {
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"
};

RegisterManager::RegisterManager() : used(tempRegs.size() + savedRegs.size(), false) {}

std::string RegisterManager::allocateTemp() {
    for (size_t i = 0; i < tempRegs.size(); ++i) {
        if (!used[i]) {
            used[i] = true;
            return tempRegs[i];
        }
    }
    return ""; // 无可用寄存器
}

std::string RegisterManager::allocateSaved() {
    for (size_t i = 0; i < savedRegs.size(); ++i) {
        size_t idx = tempRegs.size() + i;
        if (!used[idx]) {
            used[idx] = true;
            return savedRegs[i];
        }
    }
    return ""; // 无可用寄存器
}

void RegisterManager::releaseRegister(const std::string& reg) {
    int idx = getRegisterIndex(reg);
    if (idx >= 0) {
        used[idx] = false;
    }
}

void RegisterManager::releaseAllTemp() {
    for (size_t i = 0; i < tempRegs.size(); ++i) {
        used[i] = false;
    }
}

int RegisterManager::getRegisterIndex(const std::string& reg) const {
    auto it = std::find(tempRegs.begin(), tempRegs.end(), reg);
    if (it != tempRegs.end()) {
        return it - tempRegs.begin();
    }
    
    it = std::find(savedRegs.begin(), savedRegs.end(), reg);
    if (it != savedRegs.end()) {
        return tempRegs.size() + (it - savedRegs.begin());
    }
    
    return -1;
}

// RISCVCodeGenerator实现
std::string RISCVCodeGenerator::generate(CompilationUnit& unit, 
                                        const std::unordered_map<std::string, FunctionInfo>& functions) {
    functionTable = functions;
    output.str("");
    output.clear();
    
    // 生成汇编文件头部
    emit(".text");
    emit(".globl main");
    emitComment("ToyC Compiler Generated Code");
    
    // 访问编译单元
    unit.accept(*this);
    
    return output.str();
}

std::string RISCVCodeGenerator::newLabel(const std::string& prefix) {
    return prefix + std::to_string(labelCounter++);
}

void RISCVCodeGenerator::emit(const std::string& instruction) {
    output << "    " << instruction << "\n";
}

void RISCVCodeGenerator::emitLabel(const std::string& label) {
    output << label << ":\n";
}

void RISCVCodeGenerator::emitComment(const std::string& comment) {
    output << "    # " << comment << "\n";
}

std::string RISCVCodeGenerator::loadImmediate(int value, const std::string& reg) {
    if (value >= -2048 && value <= 2047) {
        emit("addi " + reg + ", zero, " + std::to_string(value));
    } else {
        // 对于大立即数，需要使用lui + addi
        int upper = (value + 0x800) >> 12;
        int lower = value & 0xfff;
        if (lower >= 2048) lower -= 4096;
        
        emit("lui " + reg + ", " + std::to_string(upper));
        if (lower != 0) {
            emit("addi " + reg + ", " + reg + ", " + std::to_string(lower));
        }
    }
    return reg;
}

void RISCVCodeGenerator::generateFunctionPrologue(const std::string& funcName, int frameSize) {
    emitComment("Function: " + funcName);
    emit("addi sp, sp, -" + std::to_string(frameSize));
    emit("sw ra, " + std::to_string(frameSize - 4) + "(sp)");
    emit("sw fp, " + std::to_string(frameSize - 8) + "(sp)");
    emit("addi fp, sp, " + std::to_string(frameSize));
}

void RISCVCodeGenerator::generateFunctionEpilogue() {
    emit("lw ra, " + std::to_string(currentFrameSize - 4) + "(sp)");
    emit("lw fp, " + std::to_string(currentFrameSize - 8) + "(sp)");
    emit("addi sp, sp, " + std::to_string(currentFrameSize));
    emit("jr ra");
}

std::string RISCVCodeGenerator::evaluateExpression(Expression& expr) {
    expr.accept(*this);
    // 表达式的结果应该在某个寄存器中，这里简化处理
    return "t0"; // 假设结果在t0中
}

int RISCVCodeGenerator::calculateFrameSize(const std::vector<Parameter>& params, Block& body) {
    std::unordered_map<std::string, int> locals;
    int offset = 0;
    collectLocalVariables(body, locals, offset);
    
    // 8字节对齐，包括ra和fp的空间
    int size = 8 + (-offset);
    return (size + 7) & ~7;
}

void RISCVCodeGenerator::collectLocalVariables(Block& body, 
                                             std::unordered_map<std::string, int>& locals, 
                                             int& offset) {
    for (auto& stmt : body.statements) {
        if (auto varDecl = dynamic_cast<VariableDeclaration*>(stmt.get())) {
            offset -= 4;
            locals[varDecl->name] = offset;
            symbolTable[varDecl->name] = Symbol(varDecl->name, Expression::INT, offset);
        } else if (auto block = dynamic_cast<Block*>(stmt.get())) {
            collectLocalVariables(*block, locals, offset);
        }
    }
}

// Visitor实现
void RISCVCodeGenerator::visit(CompilationUnit& node) {
    for (auto& func : node.functions) {
        func->accept(*this);
    }
}

void RISCVCodeGenerator::visit(FunctionDefinition& node) {
    currentFunction = node.name;
    symbolTable.clear();
    
    // 计算栈帧大小
    currentFrameSize = calculateFrameSize(node.parameters, *node.body);
    
    // 生成函数标签
    emitLabel(node.name);
    
    // 生成函数序言
    generateFunctionPrologue(node.name, currentFrameSize);
    
    // 添加参数到符号表
    int paramOffset = 8;
    for (const auto& param : node.parameters) {
        symbolTable[param.name] = Symbol(param.name, param.type, paramOffset, true);
        paramOffset += 4;
    }
    
    // 生成函数体代码
    node.body->accept(*this);
    
    // 如果是void函数且没有显式return，添加默认return
    if (node.returnType == Expression::VOID) {
        generateFunctionEpilogue();
    }
    
    emit(""); // 空行分隔
}

void RISCVCodeGenerator::visit(Block& node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void RISCVCodeGenerator::visit(NumberLiteral& node) {
    std::string reg = regManager.allocateTemp();
    loadImmediate(node.value, reg);
    // 结果存储在reg中，调用者需要知道这个寄存器
}

void RISCVCodeGenerator::visit(Identifier& node) {
    auto it = symbolTable.find(node.name);
    if (it != symbolTable.end()) {
        std::string reg = regManager.allocateTemp();
        const Symbol& symbol = it->second;
        
        if (symbol.isParameter) {
            emit("lw " + reg + ", " + std::to_string(symbol.offset) + "(fp)");
        } else {
            emit("lw " + reg + ", " + std::to_string(symbol.offset) + "(fp)");
        }
    }
}

void RISCVCodeGenerator::visit(BinaryExpression& node) {
    // 计算左操作数
    node.left->accept(*this);
    std::string leftReg = "t0"; // 假设结果在t0
    
    // 计算右操作数
    node.right->accept(*this);
    std::string rightReg = "t1"; // 假设结果在t1
    
    std::string resultReg = regManager.allocateTemp();
    
    switch (node.op) {
        case BinaryExpression::ADD:
            emit("add " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::SUB:
            emit("sub " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::MUL:
            emit("mul " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::DIV:
            emit("div " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::MOD:
            emit("rem " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::LT:
            emit("slt " + resultReg + ", " + leftReg + ", " + rightReg);
            break;
        case BinaryExpression::LE:
            emit("slt " + resultReg + ", " + rightReg + ", " + leftReg);
            emit("xori " + resultReg + ", " + resultReg + ", 1");
            break;
        case BinaryExpression::GT:
            emit("slt " + resultReg + ", " + rightReg + ", " + leftReg);
            break;
        case BinaryExpression::GE:
            emit("slt " + resultReg + ", " + leftReg + ", " + rightReg);
            emit("xori " + resultReg + ", " + resultReg + ", 1");
            break;
        case BinaryExpression::EQ:
            emit("sub " + resultReg + ", " + leftReg + ", " + rightReg);
            emit("seqz " + resultReg + ", " + resultReg);
            break;
        case BinaryExpression::NE:
            emit("sub " + resultReg + ", " + leftReg + ", " + rightReg);
            emit("snez " + resultReg + ", " + resultReg);
            break;
        case BinaryExpression::AND: {
            std::string falseLabel = newLabel("and_false");
            std::string endLabel = newLabel("and_end");
            
            emit("beqz " + leftReg + ", " + falseLabel);
            emit("beqz " + rightReg + ", " + falseLabel);
            loadImmediate(1, resultReg);
            emit("j " + endLabel);
            emitLabel(falseLabel);
            loadImmediate(0, resultReg);
            emitLabel(endLabel);
            break;
        }
        case BinaryExpression::OR: {
            std::string trueLabel = newLabel("or_true");
            std::string endLabel = newLabel("or_end");
            
            emit("bnez " + leftReg + ", " + trueLabel);
            emit("bnez " + rightReg + ", " + trueLabel);
            loadImmediate(0, resultReg);
            emit("j " + endLabel);
            emitLabel(trueLabel);
            loadImmediate(1, resultReg);
            emitLabel(endLabel);
            break;
        }
    }
    
    regManager.releaseRegister(leftReg);
    regManager.releaseRegister(rightReg);
}

void RISCVCodeGenerator::visit(UnaryExpression& node) {
    node.operand->accept(*this);
    std::string operandReg = "t0"; // 假设操作数在t0
    std::string resultReg = regManager.allocateTemp();
    
    switch (node.op) {
        case UnaryExpression::PLUS:
            emit("mv " + resultReg + ", " + operandReg);
            break;
        case UnaryExpression::MINUS:
            emit("sub " + resultReg + ", zero, " + operandReg);
            break;
        case UnaryExpression::NOT:
            emit("seqz " + resultReg + ", " + operandReg);
            break;
    }
    
    regManager.releaseRegister(operandReg);
}

void RISCVCodeGenerator::visit(AssignmentStatement& node) {
    // 计算右值
    node.value->accept(*this);
    std::string valueReg = "t0"; // 假设结果在t0
    
    // 存储到变量
    auto it = symbolTable.find(node.variable);
    if (it != symbolTable.end()) {
        const Symbol& symbol = it->second;
        emit("sw " + valueReg + ", " + std::to_string(symbol.offset) + "(fp)");
    }
    
    regManager.releaseRegister(valueReg);
}

void RISCVCodeGenerator::visit(VariableDeclaration& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
        std::string valueReg = "t0"; // 假设结果在t0
        
        auto it = symbolTable.find(node.name);
        if (it != symbolTable.end()) {
            const Symbol& symbol = it->second;
            emit("sw " + valueReg + ", " + std::to_string(symbol.offset) + "(fp)");
        }
        
        regManager.releaseRegister(valueReg);
    }
}

void RISCVCodeGenerator::visit(IfStatement& node) {
    std::string elseLabel = newLabel("if_else");
    std::string endLabel = newLabel("if_end");
    
    // 计算条件
    node.condition->accept(*this);
    std::string condReg = "t0"; // 假设条件结果在t0
    
    emit("beqz " + condReg + ", " + (node.elseStatement ? elseLabel : endLabel));
    regManager.releaseRegister(condReg);
    
    // then分支
    node.thenStatement->accept(*this);
    
    if (node.elseStatement) {
        emit("j " + endLabel);
        emitLabel(elseLabel);
        node.elseStatement->accept(*this);
    }
    
    emitLabel(endLabel);
}

void RISCVCodeGenerator::visit(WhileStatement& node) {
    std::string loopLabel = newLabel("while_loop");
    std::string endLabel = newLabel("while_end");
    
    breakLabels.push_back(endLabel);
    continueLabels.push_back(loopLabel);
    
    emitLabel(loopLabel);
    
    // 计算条件
    node.condition->accept(*this);
    std::string condReg = "t0"; // 假设条件结果在t0
    
    emit("beqz " + condReg + ", " + endLabel);
    regManager.releaseRegister(condReg);
    
    // 循环体
    node.body->accept(*this);
    
    emit("j " + loopLabel);
    emitLabel(endLabel);
    
    breakLabels.pop_back();
    continueLabels.pop_back();
}

void RISCVCodeGenerator::visit(BreakStatement& node) {
    if (!breakLabels.empty()) {
        emit("j " + breakLabels.back());
    }
}

void RISCVCodeGenerator::visit(ContinueStatement& node) {
    if (!continueLabels.empty()) {
        emit("j " + continueLabels.back());
    }
}

void RISCVCodeGenerator::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
        emit("mv a0, t0"); // 返回值放在a0寄存器
    }
    
    generateFunctionEpilogue();
}

void RISCVCodeGenerator::visit(ExpressionStatement& node) {
    node.expression->accept(*this);
    regManager.releaseAllTemp(); // 表达式语句结束后释放所有临时寄存器
}

void RISCVCodeGenerator::visit(FunctionCall& node) {
    // 保存调用者保存的寄存器
    std::vector<std::string> callerSaved = {"t0", "t1", "t2", "t3", "t4", "t5", "t6"};
    saveRegisters(callerSaved);
    
    // 准备参数（RISC-V调用约定：前8个参数通过a0-a7传递）
    for (size_t i = 0; i < node.arguments.size() && i < 8; ++i) {
        node.arguments[i]->accept(*this);
        emit("mv a" + std::to_string(i) + ", t0");
    }
    
    // 调用函数
    emit("call " + node.functionName);
    
    // 恢复调用者保存的寄存器
    restoreRegisters(callerSaved);
    
    // 如果函数有返回值，将其移动到临时寄存器
    if (node.returnType == Expression::INT) {
        std::string resultReg = regManager.allocateTemp();
        emit("mv " + resultReg + ", a0");
    }
}

void RISCVCodeGenerator::saveRegisters(const std::vector<std::string>& regs) {
    for (const auto& reg : regs) {
        emit("addi sp, sp, -4");
        emit("sw " + reg + ", 0(sp)");
    }
}

void RISCVCodeGenerator::restoreRegisters(const std::vector<std::string>& regs) {
    for (auto it = regs.rbegin(); it != regs.rend(); ++it) {
        emit("lw " + *it + ", 0(sp)");
        emit("addi sp, sp, 4");
    }
}