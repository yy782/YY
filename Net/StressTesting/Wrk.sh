cd ../../build/bin
#cd programs/yy/Net/StressTesting
# 启动服务器（4线程，LT模式）
#./Wrk.sh
./WrkHttpServer 4 1 0 0&
SERVER_PID=$!
sleep 2

# 测试 /hello 端点（最小响应）
echo "=== LT模式 /hello ==="
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello

# 测试根路径
echo "=== LT模式 / ==="
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/

# 停止服务器
kill $SERVER_PID

./WrkHttpServer 4 1 1 0&
SERVER_PID=$!
sleep 2

# 测试 /hello 端点
echo "=== ET模式 /hello ==="
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello

# 测试根路径
echo "=== ET模式 / ==="
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/

kill $SERVER_PID


# 单线程
./WrkHttpServer 1 1 0 0&
sleep 2
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello
kill $!

# 2线程
./WrkHttpServer 2 1 0 0&
sleep 2
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello
kill $!

# 4线程
./WrkHttpServer 4 1 0 0&
sleep 2
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello
kill $!

# 8线程
./WrkHttpServer 8 1 0 0&
sleep 2
wrk -c 100 -t 4 -d 30s http://127.0.0.1:8080/hello
kill $!