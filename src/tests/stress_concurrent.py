import socket
import threading
import time
import sys
import random

# Lista de sitios populares para distribuir la carga
SITES = [
    ('www.google.com', 80),
    ('www.amazon.com', 80),
    ('www.wikipedia.org', 80),
    ('www.github.com', 80),
    ('www.microsoft.com', 80),
]

def socks5_http_request(thread_id, results):
    try:
        # Seleccionar un sitio al azar
        target_host, target_port = random.choice(SITES)

        # Conectar al proxy
        s = socket.create_connection(('127.0.0.1', 1080), timeout=10)

        # Handshake SOCKS5
        s.sendall(b'\x05\x01\x02')
        response = s.recv(2)
        if response != b'\x05\x02':
            results[thread_id] = "Error en handshake"
            s.close()
            return

        # Autenticación
        s.sendall(b'\x01\x05admin\x041234')
        response = s.recv(2)
        if response != b'\x01\x00':
            results[thread_id] = "Error en autenticación"
            s.close()
            return

        # Conectar a través del proxy al objetivo
        # Formato para IPv4: CMD+RSV+ATYP+ADDR+PORT
        # CMD=1 (connect), RSV=0, ATYP=3 (domain name)
        domain_bytes = target_host.encode('ascii')
        domain_len = len(domain_bytes)

        # Solicitud de conexión al destino usando nombre de dominio
        connect_req = b'\x05\x01\x00\x03' + bytes([domain_len]) + domain_bytes + target_port.to_bytes(2, 'big')
        s.sendall(connect_req)

        # Recibir respuesta de conexión
        response = s.recv(10)
        if response[1] != 0:
            results[thread_id] = f"Error en conexión al destino: {response[1]}"
            s.close()
            return

        # Enviar petición HTTP GET
        http_request = (
            f"GET / HTTP/1.1\r\n"
            f"Host: {target_host}\r\n"
            f"User-Agent: StressTest/1.0\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()

        s.sendall(http_request)

        # Recibir y procesar la respuesta
        data = b''
        s.settimeout(5)
        try:
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                data += chunk
                # Leer solo los primeros 8KB como máximo para no sobrecargar la memoria
                if len(data) > 8192:
                    break
        except socket.timeout:
            pass

        # Cerrar la conexión
        s.close()

        # Verificar si recibimos una respuesta HTTP válida
        if b'HTTP/' in data:
            results[thread_id] = "Éxito"
        else:
            results[thread_id] = "Error: Respuesta HTTP inválida"

    except Exception as e:
        results[thread_id] = f"Error: {str(e)}"

def test_concurrent_connections(num_connections):
    print(f"Iniciando prueba con {num_connections} solicitudes HTTP concurrentes...")
    threads = []
    results = {}

    start_time = time.time()

    # Crear hilos
    for i in range(num_connections):
        t = threading.Thread(target=socks5_http_request, args=(i, results))
        threads.append(t)

    # Iniciar hilos con pequeñas pausas para no saturar inmediatamente
    for t in threads:
        t.start()
        time.sleep(0.01)

    # Esperar a que finalicen todos los hilos
    for t in threads:
        t.join()

    # Analizar resultados
    successful = sum(1 for r in results.values() if r == "Éxito")
    print(f"Prueba completada en {time.time() - start_time:.2f} segundos")
    print(f"Solicitudes exitosas: {successful}/{num_connections}")
    print(f"Tasa de éxito: {successful/num_connections*100:.2f}%")

    # Mostrar errores comunes
    errors = {}
    for r in results.values():
        if r != "Éxito":
            errors[r] = errors.get(r, 0) + 1

    if errors:
        print("\nErrores encontrados:")
        for error, count in errors.items():
            print(f"- {error}: {count} veces")

# Ejecutar prueba con diferentes cargas
for connections in [10, 50, 100, 200, 500, 1000, 2000, 5000]:
    test_concurrent_connections(connections)
    time.sleep(5)  # Pausa entre pruebas