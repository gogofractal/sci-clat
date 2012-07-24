// incluimos las librerías que nos permitirán hacer uso de SSL
#include "ssl.h"
// librería para el uso de primitivas unix
#include <unistd.h>


ssize_t client_read(BIO* bio, sms* buffer, ssize_t length) {

    ssize_t r = -1; // variable para el control de lectura

    r = BIO_read(bio, buffer, length); // leemos los datos
    
    return r;
}

ssize_t client_write(BIO* bio, sms* buffer, ssize_t length) {

    ssize_t r = -1;// variable para el control de lectura

    while (r < 0) { // leemos mientras r sea menor que 0

        r = BIO_write(bio, buffer, length); // escribimos
        if (r <= 0) {
            if (!BIO_should_retry(bio)) {// error al comprobar si hemos de leer de nuevo
                continue;
            }
        }
    }

    return r;
}