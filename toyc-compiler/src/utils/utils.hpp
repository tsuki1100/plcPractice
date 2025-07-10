#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace Utils {
    
// 颜色枚举
enum class Color {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESET
};

// 内联的简单函数
inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline bool writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return file.good();
}

inline std::string getBaseName(const std::string& filename) {
    size_t slash = filename.find_last_of("/\\");
    size_t dot = filename.find_last_of(".");
    
    size_t start = (slash == std::string::npos) ? 0 : slash + 1;
    size_t length = (dot == std::string::npos || dot <= start) ? 
                    std::string::npos : dot - start;
    
    return filename.substr(start, length);
}

inline std::string getFileExtension(const std::string& filename) {
    size_t dot = filename.find_last_of(".");
    if (dot == std::string::npos) {
        return "";
    }
    return filename.substr(dot);
}

inline std::string getDirectoryName(const std::string& filename) {
    size_t slash = filename.find_last_of("/\\");
    if (slash == std::string::npos) {
        return ".";
    }
    return filename.substr(0, slash);
}

inline std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

// 非内联函数声明
void debugPrint(const std::string& message, bool enabled = true);
void errorPrint(const std::string& message);
void warningPrint(const std::string& message);
void infoPrint(const std::string& message);

std::vector<std::string> split(const std::string& str, char delimiter);
std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);

// 路径处理
std::string normalizePath(const std::string& path);
std::string joinPath(const std::string& dir, const std::string& file);

// 文件验证
bool isValidTcFile(const std::string& filename);
bool fileExists(const std::string& filename);
size_t getFileSize(const std::string& filename);

// 字符串验证和格式化
bool isValidIdentifier(const std::string& str);
bool isNumber(const std::string& str);
std::string escapeString(const std::string& str);

// 编译器特定工具
std::string formatErrorMessage(const std::string& filename, int line, int column, 
                              const std::string& message);
std::string getSourceLine(const std::string& filename, int lineNumber);
void printSourceContext(const std::string& filename, int errorLine, int errorColumn = 0);

// 性能测量
class Timer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    
public:
    Timer();
    void start();
    void stop();
    double elapsedMilliseconds() const;
    double elapsedSeconds() const;
};

// 编译统计
struct CompilerStats {
    int totalLines = 0;
    int totalTokens = 0;
    int totalFunctions = 0;
    int totalVariables = 0;
    int totalErrors = 0;
    int totalWarnings = 0;
    double lexTime = 0.0;
    double parseTime = 0.0;
    double semanticTime = 0.0;
    double codegenTime = 0.0;
    double totalTime = 0.0;
    
    CompilerStats();
    void reset();
    void print() const;
    void addError();
    void addWarning();
};

// 命令行参数处理
bool hasOption(int argc, char* argv[], const std::string& option);
std::string getOptionValue(int argc, char* argv[], const std::string& option);
std::vector<std::string> getArguments(int argc, char* argv[]);

// 颜色输出
void printColored(const std::string& message, Color color);
void printError(const std::string& message);
void printWarning(const std::string& message);
void printSuccess(const std::string& message);
void printInfo(const std::string& message);

} // namespace Utils