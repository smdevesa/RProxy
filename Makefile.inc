# Compilador y flags
CC = gcc
CFLAGS = -D_GNU_SOURCE -O3 -std=c11 -Wno-unused-parameter -Wall -Wno-maybe-uninitialized
LDFLAGS = -lpthread