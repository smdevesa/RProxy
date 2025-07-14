import socket

SERVER = '127.0.0.1'
PORT = 8080

def auth(sock, username, password):
    username = username.encode()
    password = password.encode()
    request = bytes([0x01, len(username)]) + username + bytes([len(password)]) + password
    sock.sendall(request)
    response = sock.recv(2)
    if len(response) != 2:
        print("Error: respuesta incompleta en auth")
        return False
    version, status = response
    print(f"[AUTH] Versión: {version}, Estado: {status}")
    return status == 0x00

def send_command(sock, command, args):
    payload = ":".join(args).encode()
    request = bytes([0x01, command, len(payload)]) + payload
    sock.sendall(request)

    response = sock.recv(1024)
    if len(response) < 2:
        print("Respuesta muy corta")
        return

    version = response[0]
    status = response[1]
    msg = response[2:].split(b'\x00')[0].decode(errors='ignore') if len(response) > 2 else ''
    print(f"[CMD] Versión: {version}, Estado: {status}, Mensaje: {msg}")
    return version, status, msg

def main():
    with socket.create_connection((SERVER, PORT)) as sock:
        print("Conectado al servidor de management")

        if not auth(sock, "jsalinny", "1234"):
            print("Autenticación fallida")
            return

        print("Enviando comando DELETE_USERS (0x02)...")
        send_command(sock, 0x02, ["tizifufunella"])

main()