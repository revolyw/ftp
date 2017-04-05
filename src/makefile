all:myThread.o myCommand.o mySocket.o client.o server.o
	gcc -o server server.o mySocket.o myThread.o myCommand.o -lpthread -lcrypt
	gcc -o client client.o mySocket.o myThread.o myCommand.o -lpthread -lcrypt
myThread.o:myThread.c
	gcc -c myThread.c
myCommand.o:myCommand.c
	gcc -c myCommand.c
mySocket.o:mySocket.c
	gcc -c mySocket.c
client.o:client.c
	gcc -c client.c
server.o:server.c
	gcc -c server.c
clear:
	rm *.o
	rm server
	rm client
