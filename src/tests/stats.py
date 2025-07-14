import socket

def main():
    with socket.create_connection(("127.0.0.1", 8080)) as sock:
        # Autenticación como tfufunella:1234
        sock.sendall(bytes([0x01, 9]) + b"tizifufunella" + bytes([4]) + b"1234")
        auth_response = sock.recv(2)
        print(f"[AUTH] {auth_response}")

        if auth_response[1] != 0x00:
            print("Fallo autenticación")
            return

        # Enviar comando STATS (0x04) sin argumentos
        sock.sendall(bytes([0x01, 0x04, 0x00]))

        response = sock.recv(1024)
        version, status = response[0], response[1]
        msg = response[2:].split(b'\x00')[0].decode(errors="ignore")
        print(f"[STATS] Versión: {version}, Estado: {status}, Msg:\n{msg}")


main()