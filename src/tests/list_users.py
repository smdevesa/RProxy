import socket
import struct

MGMT_HOST = '127.0.0.1'
MGMT_PORT = 8080

VERSION = 0x01

# Comandos
MANAGEMENT_COMMAND_USERS = 0x04

def socks5_login(sock, username, password):
    user_bytes = username.encode('utf-8')
    pass_bytes = password.encode('utf-8')

    if len(user_bytes) > 255 or len(pass_bytes) > 255:
        print("[LOGIN] Usuario o contraseña demasiado largos")
        return False

    request = struct.pack("!B", VERSION)
    request += struct.pack("!B", len(user_bytes)) + user_bytes
    request += struct.pack("!B", len(pass_bytes)) + pass_bytes

    sock.sendall(request)

    response = sock.recv(2)
    if len(response) < 2:
        print("[LOGIN] Respuesta incompleta del servidor")

    ver, status = struct.unpack("!BB", response)
    if ver != VERSION:
        print(f"[LOGIN] Versión inesperada del servidor: {ver}")
        return False

    if status == 0x00:
        print("[LOGIN] Autenticación exitosa")
        return True
    else:
        print("[LOGIN] Falló la autenticación")
        return False

def send_command(sock, command_id):
    header = struct.pack("!BBH", VERSION, command_id, 0)
    sock.sendall(header)

    header_resp = sock.recv(4)
    if len(header_resp) < 4:
        print("[ERROR] Respuesta incompleta al comando")

    ver, cid, length = struct.unpack("!BBH", header_resp)

    payload = b''
    while len(payload) < length:
        chunk = sock.recv(length - len(payload))
        if not chunk:
            break
        payload += chunk

    # Mostrar siempre lo que devuelva el servidor
    print(f"[RESPUESTA] CID={hex(cid)}, LEN={length}")
    if payload:
        print(payload.decode('utf-8', errors='replace'))
    else:
        print("[RESPUESTA] (sin contenido)")

def main():
    try:
        with socket.create_connection((MGMT_HOST, MGMT_PORT)) as sock:
            if not socks5_login(sock, "admin", "1234"):
                return

            send_command(sock, MANAGEMENT_COMMAND_USERS)
    except Exception as e:
        print(f"[ERROR] No se pudo conectar al servidor: {e}")

if __name__ == "__main__":
    main()