# Proxy SOCKS5

Este proyecto es una implementación de un **proxy para el protocolo SOCKS5**, junto con un **cliente de monitoreo**.
El cliente de monitoreo permite solicitar estadísticas como así también manejar usuarios y consultar registros de acceso. 

---

## Compilación

La compilación del proyecto se realiza mediante un `Makefile` ubicado en la raíz del repositorio.

### Comandos disponibles

- `make all`: Compila todo el proyecto.
- `make clean`: Elimina los archivos generados durante la compilación.

### Estructura de directorios

Al compilar el proyecto, se crearán automáticamente las siguientes carpetas en la raíz del repositorio:

- `obj/`: Contiene los archivos objeto generados durante la compilación.
- `bin/`: Contiene los ejecutables generados:

  - `socks5v`: Ejecutable del servidor proxy SOCKS5.
  - `client`: Cliente de monitoreo.
---

## Ejecución

### Servidor Proxy

Para ejecutar el servidor proxy SOCKS5:

```bash
./bin/socks5v [ARGS]
```

Hacer `./bin/socks5v -h` para ver todas las opciones disponibles.

### Cliente de monitoreo

Para ejecutar el cliente de monitoreo:

```bash
./bin/client <host> <port> <username> <password> COMMAND [args]
```
> **Nota:** Por defecto, al correrse por primera vez el servidor SOCKS5 existirá un usuario `admin` con contraseña `1234`.

Hacer `./bin/client -h` para ver todos los comandos disponibles.

## Más detalles

Para conocer más acerca de las implementaciones consultar el archivo pdf dentro de la carpeta `doc/`.


