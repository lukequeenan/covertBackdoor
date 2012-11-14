CC = gcc
CFLAGS = -W -Wall -g
SERVER_FLAGS = -lpcap
CLIENT_FLAGS = -lpcap

SERVER = server.out
CLIENT = client.out

project: sharedLibrary.o server.c server.h client.c client.h
	$(CC) $(CFLAGS) $(SERVER_FLAGS) sharedLibrary.o server.c -o $(SERVER)
	$(CC) $(CFLAGS) $(CLIENT_FLAGS) sharedLibrary.o client.c -o $(CLIENT)

server: sharedLibrary.o server.c server.h
	$(CC) $(CFLAGS) $(SERVER_FLAGS) sharedLibrary.o server.c -o $(SERVER)

client: sharedLibrary.o client.c client.h
	$(CC) $(CFLAGS) $(CLIENT_FLAGS) sharedLibrary.o client.c -o $(CLIENT)

sharedLibrary: sharedLibrary.c sharedLibrary.h
	$(CC) $(CFLAGS) -O -c sharedLibrary.c

clean:
	rm -f *.o *.bak *.out ex