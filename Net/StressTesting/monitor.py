import subprocess
import re
import time
from datetime import datetime
import sys
#python3 monitor.py


OUTPUT_FILE = "/home/yy/programs/yy/build/rcv_window.txt"
INTERVAL = 1

def parse_ss_output():
    """只解析 8080 端口的连接"""
    result = subprocess.run(['ss', '-tni'], capture_output=True, text=True)
    lines = result.stdout.split('\n')
    
    connections = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # 匹配状态行，并且必须包含 8080
        if re.match(r'^(ESTAB|CLOSE-WAIT|FIN-WAIT|TIME-WAIT)', line):
            # 检查是否包含 8080
            if ':8080' not in line:
                i += 1
                continue
            
            # 解析状态行
            parts = line.split()
            if len(parts) >= 5:
                status = parts[0]
                send_q = parts[1]
                recv_q = parts[2]
                local = parts[3]
                remote = parts[4]
                
                # 获取下一行（TCP 详细信息）
                if i + 1 < len(lines):
                    detail_line = lines[i + 1]
                    
                    # 提取 TCP 参数
                    rcv_space_match = re.search(r'rcv_space:(\d+)', detail_line)
                    rtt_match = re.search(r'rtt:([\d.]+)', detail_line)
                    unacked_match = re.search(r'unacked:(\d+)', detail_line)
                    
                    rcv_space = rcv_space_match.group(1) if rcv_space_match else 'N/A'
                    rtt = rtt_match.group(1) if rtt_match else 'N/A'
                    unacked = unacked_match.group(1) if unacked_match else 'N/A'
                    
                    # 判断方向：本地是 8080 就是服务器端
                    if ':8080' in local:
                        direction = 'Server→Client'
                        client_port = remote.split(':')[-1]
                    else:
                        direction = 'Client→Server'
                        client_port = local.split(':')[-1]
                    
                    connections.append({
                        'timestamp': datetime.now(),
                        'direction': direction,
                        'client_port': client_port,
                        'rcv_space': int(rcv_space) if rcv_space != 'N/A' else 0,
                        'rtt': float(rtt) if rtt != 'N/A' else 0,
                        'send_q': int(send_q),
                        'recv_q': int(recv_q),
                        'status': status,
                        'unacked': int(unacked) if unacked != 'N/A' else 0,
                        'local': local,
                        'remote': remote
                    })
        i += 1
    
    return connections

def main():
    print(f"监控开始，只监控 8080 端口，输出到: {OUTPUT_FILE}")
    
    with open(OUTPUT_FILE, 'w') as f:
        f.write("=== 8080 端口接收窗口监控 ===\n")
        f.write(f"开始时间: {datetime.now()}\n")
        f.write("\n")
        f.write("=" * 100 + "\n")
        f.write(f"{'时间':<12} | {'方向':<15} | {'端口':<8} | {'rcv_space':<10} | {'Send-Q':<8} | {'RTT':<8} | {'状态':<12}\n")
        f.write("=" * 100 + "\n")
    
    last_connections = []
    while True:
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        connections = parse_ss_output()
        
        if connections:
            # 只在有连接时输出
            with open(OUTPUT_FILE, 'a') as f:
                f.write(f"\n--- {timestamp} ---\n")
                f.write(f"总连接数: {len(connections)}\n")
                
                server_to_client = [c for c in connections if c['direction'] == 'Server→Client']
                client_to_server = [c for c in connections if c['direction'] == 'Client→Server']
                
                if server_to_client:
                    avg_rcv = sum(c['rcv_space'] for c in server_to_client) / len(server_to_client)
                    sendq_gt0 = len([c for c in server_to_client if c['send_q'] > 0])
                    f.write(f"Server→Client: {len(server_to_client)}个连接, "
                            f"平均rcv_space={avg_rcv:.0f}, Send-Q>0: {sendq_gt0}\n")
                
                if client_to_server:
                    avg_rcv = sum(c['rcv_space'] for c in client_to_server) / len(client_to_server)
                    sendq_gt0 = len([c for c in client_to_server if c['send_q'] > 0])
                    f.write(f"Client→Server: {len(client_to_server)}个连接, "
                            f"平均rcv_space={avg_rcv:.0f}, Send-Q>0: {sendq_gt0}\n")
                
                f.write("-" * 80 + "\n")
                
                for conn in connections:
                    f.write(f"{timestamp:<12} | {conn['direction']:<15} | "
                            f"{conn['client_port']:<8} | {conn['rcv_space']:<10} | "
                            f"{conn['send_q']:<8} | {conn['rtt']:<8} | {conn['status']:<12}\n")
            
            # 控制台输出
            if connections != last_connections:
                print(f"\n{timestamp} | 8080端口连接数: {len(connections)}")
                for conn in connections:
                    print(f"  {conn['direction']} | {conn['client_port']} | rcv_space={conn['rcv_space']} | Send-Q={conn['send_q']}")
                last_connections = connections
        else:
            # 没有 8080 连接时的提示
            if last_connections:
                print(f"\n{timestamp} | 没有 8080 端口的连接")
                last_connections = []
        
        time.sleep(INTERVAL)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n监控已停止")
        sys.exit(0)