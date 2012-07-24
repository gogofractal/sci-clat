
#include <stdio.h>
#include <stdlib.h>
// librería para manejo de strings
#include <string.h>
// librería incluida para poder hacer complejos cálulos matemáticos
#include <math.h>
// librería para el uso de primitivas unix
#include <unistd.h>
// librería para el uso de los sockets y las correspondientes constantes
#include <sys/socket.h>
// librería para las funciones de fecha
#include <time.h>
// librería para mostrar la traza del programa
#include "trace.h"
// librería para gestionar los paquetes enviados y recibidos
#include "sms.h"
// librería para gestionar las conexiones
#include "socket.h"
// librería con las flags que utilizaremos
#include "flags.h"
// librería con las funciones del servidor
#include "servFunctions.h"
// librería las funciones de manejo de strings
#include "type.h"
// librería las funciones de manejo de strings
#include "database.h"

#define DIM 1024

/**
 * calcula la siguiente posición del array a rellenar
 */
void nextPos(user* conn, int* connPos, int* connTam, int connGrow){
    int aux = *connTam;
    PDEBUG("INFO: Buscando el siguiente elemento vacío\n");
    //buscamos el siguiente elemento vacío
    for(; *connPos < *connTam && (*(conn + *connPos)).sock > 0; (*connPos)++);
    //si hemos llenado nuestro array reservamos más memoria, suponemos un crecimiento exponencial
    if(*connPos == *connTam){
        PDEBUG("INFO: Reasignación de memoria\n");
        *connTam += connGrow;
        conn = (user*) realloc(conn, *connTam * sizeof(user));
        // completamos con -1
        memset(conn + aux, 0, *connTam);
    }
}

/**
 * Chequea si el nombre ya esta en uso
 */
int checkName(user* conn, int connTam, char* name){
    int i = 0;
    for(; i < connTam; i++){
        if((*(conn+i)).sock > 0){
            if(strcmp((*(conn+i)).name, name) == 0){
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Envia un mensaje a todos los usuarios conectados
 */
void broadcast(user* conn, int* connTam, sms msj, int socketID, sqlite3* db){
    // le ponemos el nombre real, por si acaso nos engaña el ciente
    if(socketID == 0){
        strcpy(msj.name, SERVER);
    }else{
        int aux = searchConn(conn, *connTam, socketID);
        strcpy(msj.name, (*(conn+aux)).name);
    }
    PDEBUG("INFO: Añadiendo al log");
    db_addLog(msj, &db);

    PDEBUG("DATA: Broadcasting\n");
    int i = 0;
    for(; i < *connTam; i++){
        if((*(conn+i)).sock > 0 && (*(conn+i)).prov == FINAL){
            SSL_write((*(conn+i)).ssl, &msj, sizeof(sms));
        }
    }
    PDEBUG("DATA: Broadcast terminado\n");
}

/**
 * Envia un mensaje a todos los usuarios conectados
 */
void mp(user* conn, int* connTam, sms msj, int socketID, sqlite3* db){
    // le ponemos el nombre real, por si acaso nos engaña el ciente
    int aux2;
    if(socketID == 0){
        strcpy(msj.name, SERVER);
    }else{
        aux2 = searchConn(conn, *connTam, socketID);
        strcpy(msj.name, (*(conn+aux2)).name);
    }
    PDEBUG("INFO: Añadiendo al log");
    db_addLog(msj, &db);

    msj.flag = MSJ;
    PDEBUG("DATA: MP\n");
    int aux = searchName(conn, *connTam, msj.to);
    if(aux < 0){
        PDEBUG("DATA: Usuario no encontrado\n");
        strcpy(msj.text, "Usuario no encontrado");
        SSL_write((*(conn+aux2)).ssl, &msj, sizeof(sms));
        return;
    }
    SSL_write((*(conn+aux)).ssl, &msj, sizeof(sms));
    PDEBUG("DATA: MP enviado\n");
}

/**
 * busca una conexión de la lista dado el número del socket
 */
int searchName(user* conn, int connTam, char* name){
    PDEBUG("INFO: Buscando conexión de un socket\n");
    int i = 0;
    for(; i < connTam; i++){
        if(strcmp((*(conn + i)).name, name) == 0){
            PDEBUG("INFO: Conexión del socket encontrada\n");
            return i;
        }
    }
    PDEBUG("INFO: Conexión del socket no encontrada\n");
    return -1;
}

/**
 * Envia un mensaje a todos los usuarios conectados
 */
void closeAll(user* conn, int* connTam){
    // cerramos todas las conexiones
    PDEBUG("DATA: Cerrando conexiones\n");
    int i = 0;
    for(; i < *connTam; i++){
        if((*(conn + i)).sock > 0){
            shutdown((*(conn + i)).sock, SHUT_RDWR);
        }
    }
    PDEBUG("DATA: Conexiones Cerradas\n");
}

/**
 * busca una conexión de la lista dado el número del socket
 */
int searchConn(user* conn, int connTam, int socketID){
    PDEBUG("INFO: Buscando conexión de un socket\n");
    int i = 0;
    for(; i < connTam; i++){
        if((*(conn + i)).sock == socketID){
            PDEBUG("INFO: Conexión del socket encontrada\n");
            return i;
        }
    }
    PDEBUG("INFO: Conexión del socket no encontrada\n");
    return -1;
}

/**
 * Cierra una conexión dada por el socketID
 */
int closeConn(user* conn, int* connPos, int connTam, int socketID, fd_set* connList, sqlite3* db){
    sms auxMsj;

    int aux = searchConn(conn, connTam, socketID);
    sprintf(auxMsj.text, "Usuario '%s' desconectado", (*(conn + aux)).name);
    strcpy(auxMsj.name, SERVER);
    auxMsj.flag = MSJ;
    db_addLog(auxMsj, &db);
    //cerramos el socket
    PDEBUG("DESC: Cerrando el socket\n");
    shutdown(socketID, SHUT_RDWR);
    // borramos el descritor de nuesra lista de descriptores abiertos
    PDEBUG("DESC: Eliminando el socket de la lista\n");
    FD_CLR (socketID, connList);
    // borramos el usuario de nuestro sistema
    (*(conn + aux)).sock = 0;
    *connPos = (aux < *connPos) ? socketID : *connPos;
    PDEBUG("DESC: Socket cerrado\n");
    multicast(conn, &connTam, auxMsj, 0, db, (*(conn + aux)).room);

    return 1;
}

/**
 * Ejecuta en el servidor la cadena recibida
 */
int execParams(user* conn, int connTam, char* str, int socketID, int sock, sqlite3* db, room* rooms, int dim){
    PDEBUG("INFO: Analizando lista de parámetros\n");
    char saux[10][DIM];
    bzero(saux, 10 * DIM);
    int error = 0;
    
    // tienes permisos para ejecutar comandos en el servidor???
    PDEBUG("INFO: Comprobando permisos\n");
    int aux = searchConn(conn, connTam, socketID);
    // almacenamos en la base de datos
    sms auxMsj;
    strcpy(auxMsj.name, (*(conn + aux)).name);
    strcpy(auxMsj.text, str);
    db_addLog(auxMsj, &db);
    if((*(conn + aux)).rol != ADMIN){

        PDEBUG("INFO: Usuario sin permisos\n");
        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        strcpy(auxMsj.text, "Usuario sin permisos");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
        PDEBUG("INFO: Mensaje de usuario no válido enviado al emisor\n");
        return -1;
        
    }

    PDEBUG("INFO: Usuario autorizado\n");
    str = trim(str); // eliminamos os espacios en blanco

    if(strcmp(str, "-x") == 0){ // comando de salida

        PDEBUG("EXIT: Cerrando el servidor\n");
        shutdown(sock, SHUT_RDWR); // cerramos el socket
        sms auxMsj;
        auxMsj.flag = SERV_EXIT;
        strcpy(auxMsj.name, SERVER);
        broadcast(conn, &connTam, auxMsj, socketID, db);
        closeAll(conn, &connTam); // cerramos todas las conexiones
        PDEBUG("EXIT: Cerrando la conexión con la base de datos\n");
        db_close(&db);
        PDEBUG("EXIT: Liberando espacio\n");
        free(conn);
        PDEBUG("EXIT: Cerrando el proceso\n");
        exit(0);

    }else if(sscanf(str, "--add-user %s %s %s", saux[0], saux[1], saux[2]) == 3){ // añadiendo un usuario
        //saux[0] es el nombre
        //saux[1] es el password
        //saux[2] es el rol
        PDEBUG("INFO: Añadiendo usuario 3 param\n");
        int rol = 0;

        if(strcmp(saux[2], "admin") == 0){
            rol = ADMIN;
        }else if(strcmp(saux[2], "user") == 0){
            rol = USER;
        }else{
            PDEBUG("INFO: tercer argumento no válido");

            sms auxMsj;
            auxMsj.flag = MSJ;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Rol no reconocido.");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
            return 0;
        }
        error = db_addUser(saux[0], saux[1], rol, &db);

        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        if(error == 0){
            strcpy(auxMsj.text, "Usuario creado");
        }else if(error == 1){
            strcpy(auxMsj.text, "Nombre de usuario en uso");
        }else{
            strcpy(auxMsj.text, "Error al crear el usuario");
        }
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(sscanf(str, "--add-user %s %s", saux[0], saux[1]) == 2){ // añadiendo un usuario
        // se usará el rol por defecto
        //saux[0] es el nombre
        //saux[1] es el password
        PDEBUG("INFO: Añadiendo usuario 2 param\n");
        error = db_addUser(saux[0], saux[1], USER, &db);

        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        if(error == 0){
            strcpy(auxMsj.text, "Usuario creado");
        }else if(error == 1){
            strcpy(auxMsj.text, "Nombre de usuario en uso");
        }else{
            strcpy(auxMsj.text, "Error al crear el usuario");
        }
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(sscanf(str, "--delete-user %s", saux[0]) == 1){ // borrando un usuario
        //saux[0] es el nombre
        PDEBUG("INFO: Borrando usuario\n");
        int num = 0;

        num = db_deleteUser(saux[0], &db);

        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        if(num > 0){
            PDEBUG("INFO: Usuario borrado\n");
            strcpy(auxMsj.text, "Usuario borrado");
        }else{
            PDEBUG("INFO: Usuario no encontrado\n");
            strcpy(auxMsj.text, "Usuario no encontrado");
        }
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(strcmp(str, "--list-user") == 0){ // listando usuarios
        //saux[0] es el nombre
        PDEBUG("INFO: listando usuarios\n");
        int num = 0;

        num = db_listUser(socketID, &db);

    }else if(sscanf(str, "--log %s %s", saux[0], saux[1]) == 2){ // listando usuarios
        //saux[0] es la fecha de inicio
        //saux[1] es la fecha de finalización
        PDEBUG("INFO: listando usuarios\n");
        int num = 0;
        struct tm tm1, tm2;
        long t1, t2;

        if (strptime(saux[0], "%d/%m/%Y", &tm1) == 0 || strptime(saux[1], "%d/%m/%Y", &tm2) == 0){
            PDEBUG("INFO: Formato de fecha no válido\n");
            sms auxMsj;
            auxMsj.flag = MSJ;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Formato de fecha no válido -> dd/mm/YYYY");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
            return -1;
        }

        t1 = (long)mktime(&tm1);
        t2 = (long)mktime(&tm2);

        sprintf(saux[0], " WHERE time > %ld AND time < %ld", t1, t2);

        num = db_getLogPar(socketID, &db, saux[0]);

    }else if(strcmp(str, "--log") == 0){ // listando usuarios
        PDEBUG("INFO: listando usuarios\n");
        int num = 0;

        num = db_getLog(socketID, &db);

    }else if(sscanf(str, "--add-room %s", saux[0]) == 1){ // creacion de un cana
        // saux[0] es el nombre del canal
        PDEBUG("INFO: añadiendo canales\n");

        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);

        if(addRoom(saux[0], rooms, dim) == -1){
            strcpy(auxMsj.text, "Error al crear el canal, mire el log para más información");
        }else{
            strcpy(auxMsj.text, "Canal creado");
        }

        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(sscanf(str, "--delete-room %s", saux[0]) == 1){ // creacion de un cana
        // saux[0] es el nombre del canal
        PDEBUG("INFO: borrando canal\n");

        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);

        PDEBUG("INFO: Moviendo usuarios\n");
        strcpy(saux[1], "general");
        moveAllTo(conn, &connTam, saux[0], rooms, dim, db, saux[1]);

        if(deleteRoom(saux[0], rooms, dim) == -1){
            strcpy(auxMsj.text, "Error al borrar el canal, mire el log para más información");
        }else{
            strcpy(auxMsj.text, "Canal borrado");
        }

        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(sscanf(str, "--kick %s", saux[0]) == 1){ // expulsión de un usuario
        // saux[0] es el nombre del usuario
        PDEBUG("INFO: expulsión de un usuario\n");

        //PDEBUG("INFO: Moviendo usuarios\n");
        //strcpy(saux[1], "general");

        sms auxMsj;
        auxMsj.flag = SERV_EXIT;
        strcpy(auxMsj.name, SERVER);
        strcpy(auxMsj.text, "Usuario expulsado.");
        int aux = searchName(conn, connTam, saux[0]);
        SSL_write((*(conn+aux)).ssl, &auxMsj, sizeof(sms));
        //moveAllTo(conn, &connTam, saux[0], rooms, dim, db, saux[1]);

        if(deleteRoom(saux[0], rooms, dim) == -1){
            strcpy(auxMsj.text, "Error al borrar el canal, mire el log para más información");
        }else{
            strcpy(auxMsj.text, "Canal borrado");
        }

        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else if(sscanf(str, "--list-room") == 0){ // listando de canales
        sendRoomList((*(conn + aux)).ssl, rooms, dim);
    }else{ // error, comando no válido

        PDEBUG("INFO: Comando no válido\n");
        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        strcpy(auxMsj.text, "Comando no válido");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
        PDEBUG("INFO: Mensaje de comando no válido enviado al emisor\n");
        return -1;
        
    }
    return 0;
}

/**
 * Autentificación
 */
void auth(user* conn, int* connTam, int socketID, sms msj, sqlite3* db,
                    room* rooms, int dim){
    sms auxMsj;
    char auxc[DIM];
    int guest = 0;
    PDEBUG("DATA: Autentificando\n");
    PDEBUG("INFO: ¿Usuario Registrado?\n");

    int aux = searchConn(conn, *connTam, socketID);

    if(db_userExists(msj.text, &db) <= 0){ // el usuario no es un usuario registrado

        PDEBUG("INFO: No es un usuario registrado\n");
        //dado que no es un usuario registrado, lo conectamos como invitado
        strcpy(auxc, msj.text);
        strcat(auxc, "-guest");
        guest = 1;
    }else{

        PDEBUG("INFO: Sí es un usuario registrado\n");
        /** dado que sí es un usuario registrado, le pedimos que se autentifique
         *  con el password por lo que lo indicamos con un flag
         */
        strcpy(auxc, msj.text);
        guest = 0;
    }

    PDEBUG("INFO: ¿Nombre de usuario en uso?\n");
    if(checkName(conn, *connTam, auxc) != 0){

        PDEBUG("INFO: Usuario no conectado\n");// alamcenando el nombre del usuario
        strcpy((*(conn + aux)).name, auxc);

        if(guest == 1){ // el usuario no está conectado y no requiere autentificación por contraseña
            
            // nombre de usuario libre para su uso, asignandolo...
            PDEBUG("INFO: Usuario conectado, almacenando la conexión\n");

            // metemos los datos de la conexión en nuestro array de conexiones
            (*(conn + aux)).prov = FINAL;
            (*(conn + aux)).rol = GUEST;

            PDEBUG("INFO: Enviando mensaje de bienvenida\n");
            auxMsj.flag = MSJ;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Conexión realizada\n");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

            // hacemos el broadcast para avisar de la existencia de un nuevo usuario
            PDEBUG("INFO: Solicitando el ingreso en una sala\n");
            strcpy(auxMsj.text, "Salas de chat...");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

            sendRoomList((*(conn + aux)).ssl, rooms, dim);

            PDEBUG("INFO: Pidiendo la sala\n");
            auxMsj.flag = REQ_ROOM;
            strcpy(auxMsj.text, "Seleccione la sala:");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

        }else{ // el usuario es un usuario registrado y requiere contraseña
            PDEBUG("INFO: solicitando password\n");
            // metemos los datos de la conexión en nuestro array de conexiones
            strcpy((*(conn + aux)).name, msj.text); // guardamos el nombre del usuario para luego autenticar

            auxMsj.flag = REQ_PASS;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Password: ");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
        }
    }else{
        PDEBUG("INFO: Usuario existente\n");

        auxMsj.flag = REQ_TEXT;  // le indicamos que requerimos una respuesta con texto
        strcpy(auxMsj.name, SERVER); // nos identificamos como el servidor

        strcpy(auxMsj.text, "Usuario Existente.\nUsuario: "); // establecemos el texto que queremos que se muestre

        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
    }
}

/**
 * Autentificación con contraseña
 */
void authPassword(user* conn, int* connTam, int socketID, sms msj, sqlite3* db,
                    room* rooms, int dim){
    sms auxMsj;
    int rol = 0;
    PDEBUG("DATA: Autentificando con contraseña\n");
    PDEBUG("INFO: ¿Usuario Registrado?\n");
    /** para evitar posibles ataques vamos a comprobar que el nombre del usuario
     * asociado a este socket existe
     */
    int aux = searchConn(conn, *connTam, socketID);
    PDEBUG("INFO: Comprobando usuario\n");
    if(strcmp((*(conn + aux)).name, "") != 0){
        PDEBUG("INFO: Comprobando contraseña\n");
        if((rol = db_checkUser((*(conn + aux)).name, msj.text, &db)) <= 0){
            
            PDEBUG("INFO: Usuario o/y contraseña incorrectos\n");// intento de conexión fraudulento
            strcpy((*(conn + aux)).name, "");
            auxMsj.flag = REQ_TEXT;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Usuario o/y contraseña incorrectos, Inserte un usuario");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

        }else{// conexión correcta
            
            (*(conn + aux)).prov = FINAL;
            (*(conn + aux)).rol = rol;
            PDEBUG("INFO: Enviando mensaje de bienvenida\n");
            auxMsj.flag = MSJ;
            strcpy(auxMsj.name, SERVER);
            strcpy(auxMsj.text, "Conexión realizada\n");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
            // hacemos el broadcast para avisar de la existencia de un nuevo usuario
            PDEBUG("INFO: Solicitando el ingreso en una sala\n");
            strcpy(auxMsj.text, "Salas de chat...");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

            sendRoomList((*(conn + aux)).ssl, rooms, dim);

            PDEBUG("INFO: Pidiendo la sala\n");
            auxMsj.flag = REQ_ROOM;
            strcpy(auxMsj.text, "Seleccione la sala:");
            SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

        }
    }else{ // intento de conexión fraudulento
        PDEBUG("INFO: INTENTO DE CONEXÓN FRAUDULENTO\n");
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        strcpy(auxMsj.text, "INTENTO DE CONEXIÓN FRAUDULENTO");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));
    }
}

void sendRoomList(SSL* ssl, room* rooms, int dim){
    int i = 0;
    sms auxMsj;

    PDEBUG("INFO: Salas de chat\n");

    auxMsj.flag = MSJ;
    strcpy(auxMsj.name, SERVER);
    for(; i < dim; i++){
        if(strcmp(rooms[i].name, "") == 0){
            continue;
        }
        PDEBUG("INFO: Enviando mensaje de sala\n");
        sprintf(auxMsj.text, "-- %s --", rooms[i].name);
        SSL_write(ssl, &auxMsj, sizeof(sms));
    }
}

void roomCheckIn(user* conn, int* connTam, int socketID, sms msj, sqlite3* db,
                    room* rooms, int dim){
    int room = 0;
    sms auxMsj;

    PDEBUG("INFO: Buscando la sala\n");

    room = searchRoom(msj.text, rooms, dim);

    int aux = searchConn(conn, *connTam, socketID);

    auxMsj.flag = MSJ;
    strcpy(auxMsj.name, SERVER);

    if(room < 0){ // sala no encontrada

        PDEBUG("INFO: Solicitando el ingreso en una sala\n");
        strcpy(auxMsj.text, "Sala inválida, Salas de chat...");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

        sendRoomList((*(conn + aux)).ssl, rooms, dim);

        PDEBUG("INFO: Pidiendo la sala\n");
        auxMsj.flag = REQ_ROOM;
        strcpy(auxMsj.text, "Seleccione la sala:");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

    }else{ // sala encontrada

        PDEBUG("INFO: Asignando sala y enviando mensaje de confirmación\n");

        (*(conn + aux)).room = room;

        auxMsj.flag = OK;
        strcpy(auxMsj.text, "Conectado correctamente a la sala...");
        SSL_write((*(conn + aux)).ssl, &auxMsj, sizeof(sms));

        PDEBUG("INFO: Haciendo Multicast de la nueva conexión\n");
        sprintf(auxMsj.text, "Usuario '%s' Conectado \n", (*(conn + aux)).name);
        multicast(conn, connTam, auxMsj, socketID, db, room);
        return;

    }
}

int searchRoom(char* name, room* rooms, int dim){
    int i = 0;

    PDEBUG("INFO: Buscando una sala\n");

    for(; i < dim; i++){
        if(strcmp(rooms[i].name, name) == 0){
            return i;
        }
    }

    return -1;
}

int addRoom(char* str, room* rooms, int dim){
    int i = 0;
    if(searchRoom(str, rooms, dim) == -1){

        PDEBUG("INFO: Añadiendo el canal\n");

        for(i = 0; i < dim; i++){
            if(strcmp(rooms[i].name, "") == 0){
                strcpy(rooms[i].name, str);
                return 0;
            }
        }

        PDEBUG("ERROR: Error al añadir el canal por falta de espacio\n");
        return -1;

    }else{

        PDEBUG("ERROR: Error al añadir el canal existe otro con el mismo nombre\n");
        return -1;

    }
}

int moveAllTo(user* conn, int* connTam, char* src, room* rooms, int dim, sqlite3* db, char* dst){
    int i = 0;
    int roomSRC = -1;
    int roomDST = -1;
    sms msj;
    msj.flag = MSJ;

    PDEBUG("INFO: Moviendo usuarios\n");
    
    if((roomSRC = searchRoom(src, rooms, dim)) == -1 ||
        (roomDST = searchRoom(dst, rooms, dim)) == -1){

        PDEBUG("ERROR: Error al buscar los canales el canal ya que no se encontró\n");
        return -1;

    }else{

        PDEBUG("INFO: Moviendo usuarios\n");

        for(i = 0; i < *connTam; i++){
            if((*(conn + i)).sock != 0 && (*(conn + i)).room == roomSRC){
                (*(conn + i)).room = roomDST;

                sprintf(msj.text, "Canal actual borrado, será movido a: %s\n", dst);
                SSL_write((*(conn + i)).ssl, &msj, sizeof(sms));
                sprintf(msj.text, "Usuario '%s' Conectado \n", (*(conn + i)).name);
                multicast(conn, connTam, msj, 0, db, roomDST);
            }
        }
        return 0;

    }
}

int deleteRoom(char* str, room* rooms, int dim){
    int i = 0;
    if(searchRoom(str, rooms, dim) == -1){

        PDEBUG("ERROR: Error al borrar el canal ya que no se encontró\n");
        return -1;

    }else{

        PDEBUG("INFO: Borrando el canal\n");

        for(i = 1; i < dim; i++){
            if(strcmp(rooms[i].name, str) == 0){
                strcpy(rooms[i].name, "");
                return 0;
            }
        }

        PDEBUG("ERROR: Error, canal general no puede ser borrado\n");
        return -1;

    }
}


/**
 * Envia un mensaje a todos los usuarios conectados de una sala
 */
void multicast(user* conn, int* connTam, sms msj, int socketID, sqlite3* db,
                int room){
    // le ponemos el nombre real, por si acaso nos engaña el ciente
    if(socketID == 0){
        strcpy(msj.name, SERVER);
    }else{
        int aux = searchConn(conn, *connTam, socketID);
        strcpy(msj.name, (*(conn+aux)).name);
    }
    PDEBUG("INFO: Añadiendo al log");
    db_addLog(msj, &db);

    PDEBUG("DATA: Multicasting\n");
    int i = 0;
    for(; i < *connTam; i++){
        if((*(conn+i)).sock > 0 && (*(conn+i)).prov == FINAL
                && (*(conn+i)).room == room){
            SSL_write((*(conn+i)).ssl, &msj, sizeof(sms));
        }
    }
    PDEBUG("DATA: Multicast terminado\n");
}
