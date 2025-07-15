import socket
import time

HOST = '127.0.0.1'  # IP de tu server
PORT = 1080         # Puerto del SOCKS5
MAX_CONNECTIONS = 1000000000000000000000000  # Cambiá este número según lo que quieras probar

sockets = []

for i in range(MAX_CONNECTIONS):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        sockets.append(s)
        print(f"Conexión {i+1} exitosa")
    except Exception as e:
        print(f"Fallo en conexión {i+1}: {e}")
        break

print(f"Total de conexiones exitosas: {len(sockets)}")
input("Presioná enter para cerrar las conexiones...")
for s in sockets:
    s.close()