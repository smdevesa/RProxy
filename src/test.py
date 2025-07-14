import socket
import threading
import time
import sys

def test_direct_connection(target_ip, target_port):
    """Realiza una conexión directa al servidor para comparar respuestas"""
    print("\n--- CONEXIÓN DIRECTA (SIN PROXY) ---")
    try:
        direct_sock = socket.create_connection((target_ip, target_port), timeout=5.0)

        http_request = (
            f"GET / HTTP/1.1\r\n"
            f"Host: {target_ip}\r\n"
            f"User-Agent: Mozilla/5.0\r\n"
            f"Accept: text/html\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()

        direct_sock.sendall(http_request)
        direct_data = b""

        while True:
            chunk = direct_sock.recv(4096)
            if not chunk:
                break
            direct_data += chunk

        print(f"Conexión directa - bytes recibidos: {len(direct_data)}")
        print(f"Primeros 200 bytes: {direct_data[:200].decode('utf-8', errors='ignore')}")
        direct_sock.close()
        return direct_data
    except Exception as e:
        print(f"Error en conexión directa: {e}")
        return b""

def test_proxy_external():
    server_host = '127.0.0.1'  # Proxy SOCKS
    server_port = 1080         # Puerto del proxy

    # IP y puerto del servidor externo
    target_ip = '142.251.128.46'  # IP de Google
    target_port = 80             # Puerto HTTP estándar

    print("\n--- CONEXIÓN A TRAVÉS DEL PROXY ---")
    sock = socket.create_connection((server_host, server_port))

    # Proceso de handshake SOCKS5
    print("Enviando handshake...")
    sock.sendall(b'\x05\x01\x02')
    response = sock.recv(2)
    print(f"Handshake response: {response.hex()}")

    print("Enviando autenticación...")
    sock.sendall(b'\x01\x05admin\x041234')
    response = sock.recv(2)
    print(f"Auth response: {response.hex()}")

    print("Enviando solicitud CONNECT...")
    request = b'\x05\x01\x00\x01' + socket.inet_aton(target_ip) + target_port.to_bytes(2, 'big')
    sock.sendall(request)

    response = sock.recv(10)
    print(f"Request response: {response.hex()}")

    # Petición HTTP con marca única para verificación
    verification_tag = f"X-Verification-{int(time.time())}"
    http_request = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {target_ip}\r\n"
        f"User-Agent: Mozilla/5.0\r\n"
        f"Accept: text/html\r\n"
        f"{verification_tag}: test\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()

    print(f"Enviando {len(http_request)} bytes con verificador: {verification_tag}")
    sock.sendall(http_request)

    # Recepción de datos
    sock.settimeout(5.0)
    proxy_data = b""
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            proxy_data += chunk
            print(f"Recibidos {len(chunk)} bytes")
    except socket.timeout:
        print("Timeout en la recepción")

    print(f"Total de datos recibidos: {len(proxy_data)} bytes")
    if len(proxy_data) > 0:
        print(f"Primeros 200 bytes: {proxy_data[:200].decode('utf-8', errors='ignore')}")

    # Cerramos la conexión
    sock.close()

    # También realizamos una conexión directa para comparar
    direct_data = test_direct_connection(target_ip, target_port)

    # Verificación de autenticidad
    if proxy_data and direct_data:
        print("\n--- VERIFICACIÓN DE AUTENTICIDAD ---")
        # Verificar si las respuestas son similares (comprobando encabezados HTTP)
        proxy_headers = proxy_data.split(b'\r\n\r\n')[0] if b'\r\n\r\n' in proxy_data else proxy_data
        direct_headers = direct_data.split(b'\r\n\r\n')[0] if b'\r\n\r\n' in direct_data else direct_data

        # Buscar elementos comunes en respuestas reales de servidores web
        server_headers = [b'Server:', b'Content-Type:', b'Date:']
        matches = sum(1 for header in server_headers if header in proxy_headers)

        # Verificar que nuestro encabezado de verificación NO está en la respuesta
        # (esto confirma que no es una respuesta falsificada por Python)
        verification_not_echoed = verification_tag.encode() not in proxy_data

        if matches >= 2 and verification_not_echoed:
            print("✓ La respuesta parece auténtica (contiene encabezados típicos de servidor web)")
        else:
            print("⚠ La autenticidad de la respuesta no puede verificarse")

        # Comparar respuestas
        similarity = len(set(proxy_headers.split(b'\r\n')).intersection(set(direct_headers.split(b'\r\n'))))
        print(f"Similitud con conexión directa: {similarity} encabezados coincidentes")

    print("Test completado")

if __name__ == '__main__':
    try:
        test_proxy_external()
    except Exception as e:
        print(f"Error en test: {e}")