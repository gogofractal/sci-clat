/* 
 * File:   socket.h
 * Author: alejandro
 *
 * Created on 13 de abril de 2010, 23:56
 */

#ifndef _SOCKET_H
    #define	_SOCKET_H

    #include <openssl/ssl.h>

    #ifndef NAME_LEN
        #define NAME_LEN 20     // longitud máxima del nombre
    #endif
    #ifndef DIM
        #define DIM 1024     // longitud máxima del nombre
    #endif

    //definiendo los roles
    #define GUEST 0
    #define ADMIN 1
    #define USER 2

    typedef struct usr{
        char name[NAME_LEN];    // nombre del usuario asociado al socket
        int sock;               // información referente al socket de conexión
        SSL *ssl;
        int prov;               // Flag que indica si es provisional o no la conexión
        int rol;                // flag que nos indicará el rol que le hemos asignado
        int room;               // número de la sala a la que está conectado
    }user;

    typedef struct r{
        char name[DIM];
    }room;

#endif	/* _SOCKET_H */

