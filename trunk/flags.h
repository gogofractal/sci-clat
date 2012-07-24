/* 
 * File:   flags.h
 * Author: alejandro
 *
 * Created on 14 de abril de 2010, 0:13
 */

#ifndef _FLAGS_H
    #define	_FLAGS_H

    #define REQ_TEXT 0 // request Text
    #define REQ_PASS 8 // request Password
    #define RESPONSE 1 // response
    #define OK 2 // ok
    #define REQ_AUTH 3 // request authentification
    #define CLI_EXIT 4 // desconexión del cliente
    #define SERV_EXIT 7 // desconexión del cliente
    #define MSJ 5   // indica que enviamos un mensaje
    #define SERV_ADMIN 6
    #define CHECK_PASS 9    //solicitando el chequeo de contraseña
    #define MP 10    //solicitando el chequeo de contraseña
    #define CHECK_ROOM 11 //para solicitar el acceso a una sala de chat
    #define REQ_ROOM 12 // para solicitar al cliente que seleccione una sala
    #define SERVER "server"

    #define PROV 98     // flag que indica que el usuario es provisional
    #define FINAL 99    // flag que indica que el usuario ya está autenticado


#endif	/* _FLAGS_H */

