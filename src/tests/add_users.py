import socket

def recv_all(sock, length):
    data = b""
    while len(data) < length:
        more = sock.recv(length - len(data))
        if not more:
            raise ConnectionError("Conexión cerrada prematuramente")
        data += more
    return data

def main():
    host, port = "127.0.0.1", 8080  # Cambiar según corresponda

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))

        # Login: versión=1, usr_len=5, usr='admin', pwd_len=4, pwd='1234'
        login_req = bytes([1, 5]) + b"admin" + bytes([4]) + b"1234"
        s.sendall(login_req)

        ver, status = recv_all(s, 2)
        if status != 0:
            print("Login fallido")
            return
        print("Login exitoso")

        # Construir payload args separados por ':'
        args_str = "tizifufunella:1234"
        payload = args_str.encode('utf-8')
        payload_len = len(payload)

        # Enviar comando: VERSION=1, COMMAND=0x01 (add_user), PAYLOAD_LEN, PAYLOAD
        cmd = bytes([1, 0x01, payload_len]) + payload
        s.sendall(cmd)

        # Leer respuesta: versión(1), código(1)
        ver, code = recv_all(s, 2)
        print(f"Respuesta: versión={ver} código={code}")

        # Leer payload hasta null terminator o hasta cierre (opcional)
        # Asumiendo payload textual null-terminated:
        payload_resp = b""
        while True:
            c = s.recv(1)
            if not c or c == b'\x00':
                break
            payload_resp += c

        print(payload_resp.decode(errors="replace"))

if __name__ == "__main__":
    main()