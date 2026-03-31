cd ../../build/bin
# ./10msWrk.sh
./WrkHttpServer 4 3 1 0&
SERVER_PID=$!
sleep 2
#watch -n 1 'free -h; echo "---"; ps aux --sort=-%cpu | grep -E "WrkHttpServer|wrk" | grep -v grep'
echo "===/hello ==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/hello

# 测试根路径
echo "===/==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/


echo "===/favicon.ico==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/favicon.ico

kill $SERVER_PID

sleep 2

./WrkHttpServer 4 3 1 1&
SERVER_PID=$!
sleep 2
#watch -n 1 'free -h; echo "---"; ps aux --sort=-%cpu | grep -E "WrkHttpServer|wrk" | grep -v grep'
echo "===UsePool /hello ==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/hello

# 测试根路径
echo "===UsePool /==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/


echo "===UsePool /favicon.ico==="
wrk -c 10000 -t 8 -d 30s http://127.0.0.1:8080/favicon.ico


