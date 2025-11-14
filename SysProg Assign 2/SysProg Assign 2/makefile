# Main Target
bin/shm_manager : obj/shm_manager.o obj/sem_manager.o
	$(CC) obj/shm_manager.o obj/sem_manager.o -o bin/shm_manager

bin/server : obj/server.o obj/sem_manager.o
	$(CC) obj/server.o obj/sem_manager.o -o bin/server

bin/client : obj/client.o obj/sem_manager.o
	$(CC) obj/client.o obj/sem_manager.o -o bin/client

# Object files
obj/shm_manager.o : src/shm_manager.c
	$(CC) -c src/shm_manager.c -I inc -o obj/shm_manager.o

obj/server.o : src/server.c
	$(CC) -c src/server.c -I inc -o obj/server.o

obj/client.o : src/client.c
	$(CC) -c src/client.c -I inc -o obj/client.o

obj/sem_manager.o : src/sem_manager.c
	$(CC) -c src/sem_manager.c -I inc -o obj/sem_manager.o

# Default target
all: bin/shm_manager bin/server bin/client

# Cleanup
clean:
	rm -f bin/*
	rm -f obj/*
