# cd ../../build/bin 
# #chmod +x 10ms.sh
# ./PingPongServer 4 4 1 &
# SERVER_PID=$!
# sleep 2


# TOTAL_CONN=10000000   # 总连接数
# CONN_PER_PROCESS=100000  # 每进程连接数
# PROCESS_NUM=$((TOTAL_CONN / CONN_PER_PROCESS))

# echo "Starting $PROCESS_NUM processes, each with $CONN_PER_PROCESS connections"

# for ((i=0; i<$PROCESS_NUM; i++)); do
#     ./PingPongClient 4 4096 $CONN_PER_PROCESS 10 1 &
#     sleep 0.1  # 避免同时启动造成冲击
# done

# netstat -an | grep 8080 | wc -l

# # 查看每个进程的连接数
# for pid in $(pgrep PingPongClient); do
#     echo "PID $pid: $(ls /proc/$pid/fd 2>/dev/null | wc -l) fds"
# done

# # 实时监控
# watch -n 1 'netstat -an | grep 8080 | wc -l'

# echo "All processes started. Press Ctrl+C to stop"
# wait