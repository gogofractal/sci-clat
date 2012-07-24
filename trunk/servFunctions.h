/* 
 * File:   functions.h
 * Author: alejandro
 *
 * Created on 15 de abril de 2010, 11:11
 */

#ifndef _FUNCTIONS_H
    #define	_FUNCTIONS_H

    #include <sqlite3.h>
    #include "database.h"

    // indica la siguiente posición libre del array de conexiones
    void nextPos(user*, int*, int*, int);
    // hace un broadcast de 'sms'
    void broadcast(user*, int*, sms, int, sqlite3*);
    // hace un multicast de 'sms'
    void multicast(user*, int*, sms, int, sqlite3*, int room);
    // autenticación
    void auth(user*, int*, int, sms, sqlite3*, room*, int);
    // autenticación con contraseña
    void authPassword(user*, int*, int, sms, sqlite3*, room*, int);
    // chequea si un nombre e usurio está en uso
    int checkName(user*, int, char*);
    // busca una conexión en el array dado un socket
    int searchConn(user*, int, int);
    // cierra una conexión dado un socket
    int closeConn(user*, int*, int, int, fd_set*, sqlite3*);
    // ejecuta el comando indicado en el servidor
    int execParams(user*, int, char*, int, int, sqlite3*, room*, int);
    //cierra todas las conexiones
    void closeAll(user*, int*);
    // enviando mesaje privado
    void mp(user*, int*, sms, int, sqlite3*);
    // buscar socket por nombre
    int searchName(user*, int, char*);
    // listar salas de chat
    void sendRoomList(SSL*, room*, int);
    // selección de una sala de chat
    void roomCheckIn(user*, int*, int, sms, sqlite3*, room*, int);
    // buscador de salas en el array
    int searchRoom(char*, room*, int);
    // añadimos una sala
    int addRoom(char*, room*, int);
    // borramos una sala
    int deleteRoom(char*, room*, int);
    // movemos todos los usuarios de una sala a otra
    int moveAllTo(user*, int*, char*, room*, int, sqlite3*, char*);

#endif	/* _FUNCTIONS_H */

