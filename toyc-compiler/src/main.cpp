#include <iostream>
#include <memory>
#include <fstream>
#include "ast/ast.hpp"
#include "semantic/analyzer.hpp"
#include "codegen/riscv.hpp"
#include "utils/utils.hpp"

// 外部函数声明（由flex/bison生成）
extern FILE* yyin;
extern int yyparse();
extern std::unique_ptr<CompilationUnit> root;
extern int yylineno;

void printUsage(const char* programName) {
    std::cout << "ToyC Compiler v1.0\n"
              << "Usage: " << programName << " [options] <input.tc>\n\n"
              << "Options:\n"
              << "  -o <output>  Output file (default: input.s)\n"
              << "  -v           Verbose output\n"
              << "  --ast        Print Abstract Syntax Tree\n"
              << "  --tokens     Print tokens (lexical analysis only)\n"
              << "  --parse-only Only perform parsing\n"
              << "  --help       Show this help\n\n"
              << "Examples:\n"
              << "  " << programName << " hello.tc\n"
              << "  " << programName << " -v --ast factorial.tc -o factorial.s\n";
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile;
    bool verbose = false;
    bool printAST = false;
    bool parseOnly = false;
    
    // 简单的参数解析
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "--ast") {
            printAST = true;
        } else if (arg == "--parse-only") {
            parseOnly = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg[0] != '-') {
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files specified" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // 检查输入文件扩展名
    if (Utils::getFileExtension(inputFile) != ".tc") {
        std::cerr << "Warning: Input file should have .tc extension" << std::endl;
    }
    
    // 设置默认输出文件名
    if (outputFile.empty()) {
        outputFile = Utils::getBaseName(inputFile) + ".s";
    }
    
    try {
        if (verbose) {
            std::cout << "ToyC Compiler v1.0" << std::endl;
            std::cout << "Input file: " << inputFile << std::endl;
            std::cout << "Output file: " << outputFile << std::endl;
            std::cout << "===================" << std::endl;
        }
        
        // 1. 词法和语法分析
        if (verbose) std::cout << "Phase 1: Parsing..." << std::endl;
        
        FILE* inputFp = fopen(inputFile.c_str(), "r");
        if (!inputFp) {
            std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
            return 1;
        }
        
        yyin = inputFp;
        yylineno = 1;
        root.reset(); // 确保之前的AST被清理
        
        int parseResult = yyparse();
        fclose(inputFp);
        
        if (parseResult != 0) {
            std::cerr << "Error: Parsing failed" << std::endl;
            return 1;
        }
        
        if (!root) {
            std::cerr << "Error: No AST generated" << std::endl;
            return 1;
        }
        
        if (verbose) std::cout << "  Parsing completed successfully" << std::endl;
        
        // 打印AST（如果需要）
        if (printAST) {
            std::cout << "\n=== Abstract Syntax Tree ===" << std::endl;
            root->print();
            std::cout << "============================\n" << std::endl;
        }
        
        // 如果只需要解析，则在此结束
        if (parseOnly) {
            std::cout << "Parse-only mode: Parsing successful!" << std::endl;
            return 0;
        }
        
        // 2. 语义分析
        if (verbose) std::cout << "Phase 2: Semantic analysis..." << std::endl;
        
        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(*root)) {
            std::cerr << "Semantic analysis failed:" << std::endl;
            const auto& errors = analyzer.getErrors();
            for (size_t i = 0; i < errors.size(); ++i) {
                std::cerr << "  Error " << (i+1) << ": " << errors[i] << std::endl;
            }
            return 1;
        }
        
        if (verbose) std::cout << "  Semantic analysis completed successfully" << std::endl;
        
        // 3. 代码生成
        if (verbose) std::cout << "Phase 3: Code generation..." << std::endl;
        
        RISCVCodeGenerator generator;
        
        // 构建函数表（从AST中提取）
        std::unordered_map<std::string, FunctionInfo> functionTable;
        for (const auto& func : root->functions) {
            std::vector<Expression::Type> paramTypes;
            for (const auto& param : func->parameters) {
                paramTypes.push_back(param.type);
            }
            functionTable[func->name] = FunctionInfo(func->name, func->returnType, paramTypes, true);
        }
        
        std::string assemblyCode = generator.generate(*root, functionTable);
        
        if (verbose) std::cout << "  Code generation completed" << std::endl;
        
        // 4. 写入输出文件
        if (verbose) std::cout << "Phase 4: Writing output..." << std::endl;
        
        if (!Utils::writeFile(outputFile, assemblyCode)) {
            std::cerr << "Error: Cannot write to output file: " << outputFile << std::endl;
            return 1;
        }
        
        if (verbose) {
            std::cout << "  Output written to: " << outputFile << std::endl;
            std::cout << "===================" << std::endl;
        }
        
        std::cout << "Compilation successful!" << std::endl;
        
        // 显示统计信息
        if (verbose) {
            std::cout << "\nStatistics:" << std::endl;
            std::cout << "  Functions: " << root->functions.size() << std::endl;
            
            // 计算总行数
            std::string sourceCode = Utils::readFile(inputFile);
            int lineCount = std::count(sourceCode.begin(), sourceCode.end(), '\n') + 1;
            std::cout << "  Source lines: " << lineCount << std::endl;
            std::cout << "  Assembly lines: " << std::count(assemblyCode.begin(), assemblyCode.end(), '\n') << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: Unknown error occurred" << std::endl;
        return 1;
    }
}