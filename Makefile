# Makefile for 9menu.
#
# Edit to taste
#
# Arnold Robbins
# arnold@skeeve.atl.ga.us

CC = gcc
CFLAGS = -g -O
LIBS = -lX11

9menu: 9menu.c
	$(CC) $(CFLAGS) 9menu.c $(LIBS) -o 9menu
