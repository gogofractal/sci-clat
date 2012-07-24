/* 
 * File:   sms.h
 * Author: alejandro
 *
 * Created on 13 de abril de 2010, 23:45
 */

#ifndef _SMS_H
    #define	_SMS_H
    
    #include <time.h>

    #define SMS_LEN 1024    // longitud máxima del mensaje
    #ifndef NAME_LEN
        #define NAME_LEN 20     // longitud máxima del nombre
    #endif

    typedef struct message{
        char text[SMS_LEN];     //mensaje
        time_t time;            //hora del mensaje
        char name[NAME_LEN];    //nombre del que envía el mensaje
        char to[NAME_LEN];       // nombre del destinatario
        int flag;               // flags para comunicar acciones a reaizar
    }sms;

#endif	/* _SMS_H */

