#!/bin/bash

cd "build/bin"


echo "正在启动"

if [ ! -f "EchoServer" ]; then
    echo "错误:文件不存在！"
    exit 1
fi

if [ ! -f "ManyEchoClients" ]; then
    echo "错误:文件不存在！"
    exit 1
fi


./EchoServer


sleep 1

./EchoClient << EOF


echo "程序已退出"


# ./start.sh