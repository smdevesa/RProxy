# Proxy SOCKS5

Este proyecto es una implementación de un **proxy para el protocolo SOCKS5**.

## Compilación

La compilación del proyecto se realiza mediante un `Makefile` ubicado en la raíz del repositorio.

### Comandos disponibles

- `make all`: Compila todo el proyecto.
- `make clean`: Elimina los archivos generados durante la compilación.

### Estructura de directorios

Al compilar el proyecto, se crearán automáticamente dos carpetas en la raíz del proyecto (al mismo nivel que `src/`):

- `obj/`: Contiene los archivos objeto generados durante la compilación.
- `bin/`: Contiene el ejecutable resultante llamado `server`.

## Ejecución

Para ejecutar el proxy, usar el siguiente comando desde la raíz del proyecto:

```bash
./bin/server
