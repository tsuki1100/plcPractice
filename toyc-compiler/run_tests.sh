#!/bin/bash

COMPILER=$1
TEST_DIR="test_samples"
TEMP_DIR="/tmp/toyc_test_$$"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ -z "$COMPILER" ]; then
    echo "Usage: $0 <compiler_path>"
    exit 1
fi

if [ ! -x "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler not found or not executable: $COMPILER${NC}"
    exit 1
fi

mkdir -p "$TEMP_DIR"

echo -e "${BLUE}ToyC Compiler Test Suite${NC}"
echo "=========================="
echo "Compiler: $COMPILER"
echo "Test directory: $TEST_DIR"
echo "Temp directory: $TEMP_DIR"
echo ""

# 测试计数器
total_tests=0
passed_tests=0

# 运行单个测试
run_test() {
    local test_file=$1
    local test_name=$(basename "$test_file" .tc)
    
    echo -n "Testing $test_name... "
    total_tests=$((total_tests + 1))
    
    # 编译测试
    local output_file="$TEMP_DIR/$test_name.s"
    if "$COMPILER" "$test_file" -o "$output_file" 2>"$TEMP_DIR/$test_name.err"; then
        if [ -f "$output_file" ] && [ -s "$output_file" ]; then
            echo -e "${GREEN}PASS${NC}"
            passed_tests=$((passed_tests + 1))
            
            # 显示生成的汇编代码行数
            local line_count=$(wc -l < "$output_file")
            echo "  Generated $line_count lines of assembly"
        else
            echo -e "${RED}FAIL${NC} (empty output)"
            echo "  Error: Output file is empty or not created"
        fi
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Compilation failed for $test_file"
        if [ -f "$TEMP_DIR/$test_name.err" ]; then
            echo "  Error output:"
            sed 's/^/    /' "$TEMP_DIR/$test_name.err"
        fi
    fi
}

# 运行特殊测试（AST打印）
run_ast_test() {
    local test_file=$1
    local test_name=$(basename "$test_file" .tc)
    
    echo -n "Testing $test_name (AST)... "
    
    if "$COMPILER" "$test_file" --ast >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        echo "  AST generation successful"
    else
        echo -e "${RED}FAIL${NC}"
        echo "  AST generation failed"
    fi
}

# 运行语法错误测试
test_syntax_errors() {
    echo ""
    echo "Testing syntax error handling..."
    
    # 创建一个语法错误的测试文件
    cat > "$TEMP_DIR/syntax_error.tc" << 'EOF'
int main() {
    int x = ;  // 语法错误
    return x;
}
EOF
    
    echo -n "Testing syntax error detection... "
    if "$COMPILER" "$TEMP_DIR/syntax_error.tc" -o "$TEMP_DIR/syntax_error.s" 2>/dev/null; then
        echo -e "${RED}FAIL${NC} (should have failed)"
    else
        echo -e "${GREEN}PASS${NC}"
    fi
}

# 主测试循环
echo "Running compilation tests:"
for test_file in "$TEST_DIR"/*.tc; do
    if [ -f "$test_file" ]; then
        run_test "$test_file"
    fi
done

# AST测试
echo ""
echo "Running AST tests:"
for test_file in "$TEST_DIR"/*.tc; do
    if [ -f "$test_file" ]; then
        run_ast_test "$test_file"
    fi
done

# 语法错误测试
test_syntax_errors

echo ""
echo "=========================="
echo -e "Tests completed: ${GREEN}$passed_tests${NC}/${total_tests} passed"

# 显示失败的测试
failed_tests=$((total_tests - passed_tests))
if [ $failed_tests -gt 0 ]; then
    echo -e "${RED}$failed_tests tests failed${NC}"
    echo ""
    echo "Generated files are in: $TEMP_DIR"
    echo "To clean up: rm -rf $TEMP_DIR"
else
    echo -e "${GREEN}All tests passed!${NC}"
    # 清理临时文件
    rm -rf "$TEMP_DIR"
fi

# 返回适当的退出码
if [ $passed_tests -eq $total_tests ]; then
    exit 0
else
    exit 1
fi