/* 
 * File:   ssl.h
 * Author: alejandro
 *
 * Created on 12 de mayo de 2010, 0:16
 */

#ifndef _SSL_H
    #define	_SSL_H

    // incluimos las librerías que nos permitirán hacer uso de SSL
    #include <openssl/bio.h>
    #include <openssl/ssl.h>
    #include <openssl/err.h>
    // librería para gestionar los paquetes enviados y recibidos
    #include "sms.h"

    ssize_t client_read(BIO*, sms*, ssize_t);
    ssize_t client_write(BIO*, sms*, ssize_t);

#endif	/* _SSL_H */

