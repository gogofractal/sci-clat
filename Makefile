
do: console

all:	console
	@echo "Compilación de la práctica"

console:
	gcc -lpthread -ldl -lssl -lcrypto -lsqlite3 -Wall -o serv5 serv5.c servFunctions.c database.c type.c tools.c
	gcc  -lssl -Wall -o cli5 cli5.c type.c tools.c ssl.c
	gcc -lpthread -ldl -lssl -lcrypto -lsqlite3 -Wall -o createDB createDB.c database.c tools.c type.c
	gcc -Wall -o interfaz interfaz.c type.c
clean:
	@echo "Limpiando..."
