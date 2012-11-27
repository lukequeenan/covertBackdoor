CC = gcc -W -Wall -g -lpcap

SERVER = server.out
CLIENT = client.out

project: sharedLibrary.o functionality.o server.c server.h client.c client.h
	$(CC) functionality.o sharedLibrary.o server.c -o $(SERVER)
	$(CC) sharedLibrary.o client.c -o $(CLIENT)

server: sharedLibrary.o functionality.o server.c server.h
	$(CC) functionality.o sharedLibrary.o server.c -o $(SERVER)

client: sharedLibrary.o client.c client.h
	$(CC) sharedLibrary.o client.c -o $(CLIENT)

sharedLibrary: sharedLibrary.c sharedLibrary.h
	$(CC) -O -c sharedLibrary.c

functionality: functionality.c functionality.h
	$(CC) -O -c functionality.c

clean:
	rm -f *.o *.bak *.out ex
