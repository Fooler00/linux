#!/bin/bash

# 定义变量
SOURCE_FILE="backend.cpp"      # 你的C++源代码文件名
OUTPUT_FILE="backup_server"    # 编译后生成的可执行文件名
PORT=8080                      # 服务器运行端口

echo "=========================================="
echo "   数据备份系统 - 一键启动脚本"
echo "=========================================="

# 1. 检查源代码是否存在
if [ ! -f "$SOURCE_FILE" ]; then
    echo "[错误] 找不到源代码文件: $SOURCE_FILE"
    echo "请确保当前目录下有 $SOURCE_FILE 文件。"
    exit 1
fi

# 2. 编译代码
echo "[步骤 1/3] 正在编译 $SOURCE_FILE ..."
g++ -std=c++17 "$SOURCE_FILE" -o "$OUTPUT_FILE" -lssl -lcrypto -lpthread

# 检查编译是否成功
if [ $? -ne 0 ]; then
    echo "[错误] 编译失败！请检查代码中的语法错误。"
    exit 1
fi
echo "[成功] 编译完成，生成可执行文件: $OUTPUT_FILE"

# 3. 检查端口是否被占用
if command -v ss &> /dev/null; then
    if ss -tuln | grep ":$PORT" &> /dev/null; then
        echo "[警告] 端口 $PORT 已被占用，正在尝试关闭旧进程..."
        # 尝试杀掉占用该端口的进程（可选）
        fuser -k $PORT/tcp 2>/dev/null
        sleep 1
    fi
fi

# 4. 启动服务器
echo "[步骤 2/3] 正在启动服务器..."
echo "[步骤 3/3] 请在浏览器中访问: http://localhost:$PORT"
echo "按 Ctrl+C 可停止服务器。"
echo "=========================================="

# 执行编译好的程序
./"$OUTPUT_FILE"