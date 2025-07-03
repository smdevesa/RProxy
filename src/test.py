import socket

sock = socket.create_connection(('127.0.0.1', 1080))

# Handshake: version 5, 1 method, username/password (0x02)
sock.sendall(b'\x05\x01\x02')
print("Handshake response:", sock.recv(2))

# Auth: version 1, username length + username, password length + password
sock.sendall(b'\x01\x05admin\x05admin')
print("Auth response:", sock.recv(2))

sock.close()