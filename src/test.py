import socket

def main():
    server_host = '127.0.0.1'
    server_port = 1080

    sock = socket.create_connection((server_host, server_port))

    # -------- HANDSHAKE --------
    # Versión 5, 1 método, USERNAME/PASSWORD (0x02)
    sock.sendall(b'\x05\x01\x02')
    response = sock.recv(2)
    print(f"Handshake response: {response.hex()}")  # debería ser 05 02

    # -------- AUTHENTICATION --------
    # Versión 1, username 'admin', password 'admin'
    sock.sendall(b'\x01\x05admin\x05admin')
    response = sock.recv(2)
    print(f"Auth response: {response.hex()}")  # debería ser 01 00

    # -------- REQUEST --------
    # Versión 5, comando CONNECT, RSV 0x00, IPv4, destino 1.2.3.4 puerto 8080
    request = b'\x05\x01\x00\x01' + b'\x01\x02\x03\x04' + b'\x1f\x90'  # 0x1f90 = 8080
    sock.sendall(request)

    # Recibir respuesta del servidor
    response = sock.recv(10)
    print(f"Request response: {response.hex()}")  # Imprime la respuesta en formato hexadecimal

    sock.close()

if __name__ == '__main__':
    main()