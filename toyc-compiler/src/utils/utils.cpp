#include "utils/utils.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace Utils {
    
// 调试和日志函数
void debugPrint(const std::string& message, bool enabled) {
    if (enabled) {
        std::cerr << "[DEBUG] " << message << std::endl;
    }
}

void errorPrint(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void warningPrint(const std::string& message) {
    std::cerr << "[WARNING] " << message << std::endl;
}

void infoPrint(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

// 字符串处理函数的非内联版本（更复杂的实现）
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : str) {
        if (c == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

// 路径处理函数的更健壮版本
std::string normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // 统一使用正斜杠
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // 去掉末尾的斜杠（除非是根目录）
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized;
}

std::string joinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    if (file.empty()) return dir;
    
    std::string result = normalizePath(dir);
    if (result.back() != '/') {
        result += '/';
    }
    result += file;
    
    return result;
}

// 文件验证函数
bool isValidTcFile(const std::string& filename) {
    // 检查文件扩展名
    if (getFileExtension(filename) != ".tc") {
        return false;
    }
    
    // 检查文件名是否为空
    std::string baseName = getBaseName(filename);
    if (baseName.empty()) {
        return false;
    }
    
    // 检查文件名是否包含非法字符
    for (char c : baseName) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            return false;
        }
    }
    
    return true;
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

size_t getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        return 0;
    }
    return static_cast<size_t>(file.tellg());
}

// 字符串格式化和验证
bool isValidIdentifier(const std::string& str) {
    if (str.empty()) return false;
    
    // 第一个字符必须是字母或下划线
    if (!std::isalpha(str[0]) && str[0] != '_') {
        return false;
    }
    
    // 其他字符必须是字母、数字或下划线
    for (size_t i = 1; i < str.length(); ++i) {
        if (!std::isalnum(str[i]) && str[i] != '_') {
            return false;
        }
    }
    
    return true;
}

bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.length() == 1) return false;
        start = 1;
    }
    
    for (size_t i = start; i < str.length(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    
    return true;
}

std::string escapeString(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            case '\r': escaped += "\\r"; break;
            case '\\': escaped += "\\\\"; break;
            case '\"': escaped += "\\\""; break;
            case '\'': escaped += "\\'"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

// 编译器特定的工具函数
std::string formatErrorMessage(const std::string& filename, int line, int column, 
                              const std::string& message) {
    std::string result = filename;
    if (line > 0) {
        result += ":" + std::to_string(line);
        if (column > 0) {
            result += ":" + std::to_string(column);
        }
    }
    result += ": " + message;
    return result;
}

std::string getSourceLine(const std::string& filename, int lineNumber) {
    std::ifstream file(filename);
    if (!file) {
        return "";
    }
    
    std::string line;
    int currentLine = 1;
    
    while (std::getline(file, line) && currentLine <= lineNumber) {
        if (currentLine == lineNumber) {
            return line;
        }
        currentLine++;
    }
    
    return "";
}

void printSourceContext(const std::string& filename, int errorLine, int errorColumn) {
    if (errorLine <= 0) return;
    
    // 打印错误行的前一行（如果存在）
    if (errorLine > 1) {
        std::string prevLine = getSourceLine(filename, errorLine - 1);
        if (!prevLine.empty()) {
            std::cout << std::setw(4) << (errorLine - 1) << " | " << prevLine << std::endl;
        }
    }
    
    // 打印错误行
    std::string errorLineStr = getSourceLine(filename, errorLine);
    if (!errorLineStr.empty()) {
        std::cout << std::setw(4) << errorLine << " | " << errorLineStr << std::endl;
        
        // 打印错误位置指示器
        if (errorColumn > 0) {
            std::cout << "     | ";
            for (int i = 1; i < errorColumn; ++i) {
                std::cout << " ";
            }
            std::cout << "^" << std::endl;
        }
    }
    
    // 打印错误行的后一行（如果存在）
    std::string nextLine = getSourceLine(filename, errorLine + 1);
    if (!nextLine.empty()) {
        std::cout << std::setw(4) << (errorLine + 1) << " | " << nextLine << std::endl;
    }
}

// 性能测量工具
Timer::Timer() {
    start();
}

void Timer::start() {
    startTime = std::chrono::high_resolution_clock::now();
}

void Timer::stop() {
    endTime = std::chrono::high_resolution_clock::now();
}

double Timer::elapsedMilliseconds() const {
    auto end = (endTime.time_since_epoch().count() == 0) ? 
               std::chrono::high_resolution_clock::now() : endTime;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - startTime);
    return duration.count() / 1000.0;
}

double Timer::elapsedSeconds() const {
    return elapsedMilliseconds() / 1000.0;
}

// 编译统计信息
CompilerStats::CompilerStats() {
    reset();
}

void CompilerStats::reset() {
    totalLines = 0;
    totalTokens = 0;
    totalFunctions = 0;
    totalVariables = 0;
    totalErrors = 0;
    totalWarnings = 0;
    lexTime = 0.0;
    parseTime = 0.0;
    semanticTime = 0.0;
    codegenTime = 0.0;
    totalTime = 0.0;
}

void CompilerStats::print() const {
    std::cout << "\n=== Compilation Statistics ===" << std::endl;
    std::cout << "Source Analysis:" << std::endl;
    std::cout << "  Total lines: " << totalLines << std::endl;
    std::cout << "  Total tokens: " << totalTokens << std::endl;
    std::cout << "  Functions defined: " << totalFunctions << std::endl;
    std::cout << "  Variables declared: " << totalVariables << std::endl;
    
    std::cout << "\nError Summary:" << std::endl;
    std::cout << "  Errors: " << totalErrors << std::endl;
    std::cout << "  Warnings: " << totalWarnings << std::endl;
    
    std::cout << "\nTiming Information:" << std::endl;
    std::cout << "  Lexical analysis: " << std::fixed << std::setprecision(2) 
              << lexTime << " ms" << std::endl;
    std::cout << "  Parsing: " << parseTime << " ms" << std::endl;
    std::cout << "  Semantic analysis: " << semanticTime << " ms" << std::endl;
    std::cout << "  Code generation: " << codegenTime << " ms" << std::endl;
    std::cout << "  Total time: " << totalTime << " ms" << std::endl;
    
    if (totalTime > 0) {
        std::cout << "\nPerformance:" << std::endl;
        std::cout << "  Lines per second: " 
                  << static_cast<int>(totalLines * 1000.0 / totalTime) << std::endl;
    }
    
    std::cout << "=============================" << std::endl;
}

void CompilerStats::addError() {
    totalErrors++;
}

void CompilerStats::addWarning() {
    totalWarnings++;
}

// 命令行参数解析辅助函数
bool hasOption(int argc, char* argv[], const std::string& option) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == option) {
            return true;
        }
    }
    return false;
}

std::string getOptionValue(int argc, char* argv[], const std::string& option) {
    for (int i = 1; i < argc - 1; ++i) {
        if (argv[i] == option) {
            return argv[i + 1];
        }
    }
    return "";
}

std::vector<std::string> getArguments(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return args;
}

// 颜色输出辅助函数
void printColored(const std::string& message, Color color) {
    const char* colorCode = "";
    switch (color) {
        case Color::RED:    colorCode = "\033[31m"; break;
        case Color::GREEN:  colorCode = "\033[32m"; break;
        case Color::YELLOW: colorCode = "\033[33m"; break;
        case Color::BLUE:   colorCode = "\033[34m"; break;
        case Color::MAGENTA:colorCode = "\033[35m"; break;
        case Color::CYAN:   colorCode = "\033[36m"; break;
        case Color::WHITE:  colorCode = "\033[37m"; break;
        default:            colorCode = "\033[0m";  break;
    }
    
    std::cout << colorCode << message << "\033[0m";
}

void printError(const std::string& message) {
    printColored("[ERROR] " + message, Color::RED);
    std::cout << std::endl;
}

void printWarning(const std::string& message) {
    printColored("[WARNING] " + message, Color::YELLOW);
    std::cout << std::endl;
}

void printSuccess(const std::string& message) {
    printColored("[SUCCESS] " + message, Color::GREEN);
    std::cout << std::endl;
}

void printInfo(const std::string& message) {
    printColored("[INFO] " + message, Color::BLUE);
    std::cout << std::endl;
}

} // namespace Utils