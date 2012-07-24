/**
 * @File: cli5.c
 * @Description: implements the client
 * @Group: 1
 * @Members:    Alejandro Escario Méndez
 *              Alejandro Fernández Alderete
 *              Carlos Martínez Rey
 */

#include <stdio.h>
#include <stdlib.h>
// librería para manejo de strings
#include <string.h>
// librería para el uso de los sockets y las correspondientes constantes
#include <sys/socket.h>
// librería para el uso de la constante IPPROTO_TCP, in_addr, ...
#include <netinet/in.h>
// librería que nos permite hacer uso de la variable errno
#include <errno.h>
// librería de base de datos de red
#include <netdb.h>
// librería para el uso de primitivas unix
#include <unistd.h>
// librería para obtener acceso a variables como pid
#include <sys/types.h>
// librería para el manejo de señales
#include <signal.h>
// librería para mostrar la traza del programa
#include "trace.h"
// librería para gestionar los paquetes enviados y recibidos
#include "sms.h"
// librería con las flags que utilizaremos
#include "flags.h"
// incuimos la librería de tipos de datos
#include "type.h"
// incuimos la librería de herramientas para los echo
#include "tools.h"
// incluimos las librerías que nos permitirán hacer uso de SSL
#include "ssl.h"

#define DIM 1024 // definimos el tamaño de los array de textos

#define     READ	STDIN_FILENO
#define     WRITE	STDOUT_FILENO

int main(int argc, char** argv){

    int sock = 0; // declaración del socket e inicializado a 0
    int error = 0; /** declaramos una variable que nos servirá para detectar
                    * errores
                    */
    int serverTalk = 0;
    //socklen_t length = (socklen_t) sizeof (struct sockaddr_in); // tamaño del paquete
    //struct sockaddr_in addr; // definimos el contenedor de la dirección
    unsigned int port = 5678; /** creamos la variable que identifica el puerto
                               * de conexión, siendo el puerto por defecto 5678
                               */
    char dir[DIM] = "localhost"; /** definimos la cadena que contendrá a la
                                  * dirección del servidor, la cual, será por
                                  * defecto localhost
                                  */
    //struct hostent*  server; // estructura utilizada para la gestión de direcciones
    sms auxMsj;
    char name[DIM] = {0};
    int nbytes = 0; // contador de bytes leidos y escritos
    char aux[DIM] = {0};
    // inicializamos las variables de SSL
    BIO* bio = NULL;
    SSL_CTX* ctx = NULL;
    SSL* ssl = NULL;
    char cert[DIM] = "/usr/share/doc/libssl-dev/demos/sign/cert.pem";

    //analizamos los parámetros de entrada
    int i = 0;
    for(; i < argc; i++){
        if(strcmp(argv[i], "-p") == 0){ // leemos el puerto
            if(argc <= i + 1 || isNum(argv[i+1]) == 0){
                perror("Se esperaba un número después de -p");
                exit(-1);
            }else{
                PDEBUG("INFO: Puerto identificado\n");
                i++;
                port = atoi(argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-d") == 0){ // dirección de destino
            if(argc <= i + 1){
                perror("Se esperaba una dirección después de -d");
                exit(-1);
            }else{
                PDEBUG("INFO: Destino identificado");
                i++;
                strcpy(dir, argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-cert") == 0){ // dirección de destino
            if(argc <= i + 1){
                perror("Se esperaba una ruta de certificado después de -cert");
                exit(-1);
            }else{
                PDEBUG("INFO: Destino identificado");
                i++;
                strcpy(cert, argv[i]);
            }
            continue;
        }
    }

    /***********************************SSL************************************/
    PDEBUG("INFO: Iniciamos la librería SSL\n");
    SSL_load_error_strings(); // strings de error
    SSL_library_init(); // iniciams la libreria en sí
    ERR_load_BIO_strings(); // strings de error de BIO
    OpenSSL_add_all_algorithms(); // inicializamos los algoritmos de la librería

    PDEBUG("INFO: Conectando...\n");
    PDEBUG("INFO: Inicializando los punteros\n");
    ctx = SSL_CTX_new(SSLv23_client_method());
    ssl = NULL;
    PDEBUG("INFO: Cargamos el certificado\n");
    if(SSL_CTX_load_verify_locations(ctx, cert, NULL) == 0){
        char aux[] = "ERROR: No se pudo comprobar el certificado\n";
        write(WRITE, aux, strlen(aux));
        exit(-1);
    }
    PDEBUG("INFO: Inicializando BIO\n");
    bio = BIO_new_ssl_connect(ctx);
    BIO_get_ssl(bio, &ssl);
    if(ssl == 0){
        char aux[] = "ERROR: Error al crear el objeto ssl\n";
        write(WRITE, aux, strlen(aux));
        exit(-1);
    }
    PDEBUG("INFO: Estableciendo el modo de trabajo, no queremos reintentos\n");
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    PDEBUG("INFO: Intentando realizar la conexión\n");
    PDEBUG("INFO: Conectando a -> ");
    sprintf(aux, "%s:%i", dir, port);
    PDEBUG(aux);PDEBUG("\n");
    BIO_set_conn_hostname(bio, aux);
    PDEBUG("INFO: Verificando la conexión\n");
    if (BIO_do_connect(bio) < 1){
        char aux[] = "ERROR: al conectar el BIO\n";
        write(WRITE, aux, strlen(aux));
        exit(-1);
    }
    //PDEBUG("INFO: Verificando el resultado de la conexión\n");
    //printf("--%i-%i--\n", X509_V_OK, SSL_get_verify_result(ssl));
    //if (SSL_get_verify_result(ssl) != X509_V_OK) {
    //    char aux[] = "ERROR: verificar el resultado de la conexión\n";
    //    write(WRITE, aux, strlen(aux));
    //    exit(-1);
    //}

    PDEBUG("INFO: Conectado\n");
    /***********************************SSL************************************/

    ////Creamos el socket
    //PDEBUG("INFO: Creando el socket\n");
    //sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    ////Comprobamos si ha ocurrido un error al crear el socket
    //if(sock < 0){
    //    char aux[] = "ERROR: creación del socket {{socket()}}:\n";
    //    write(WRITE, aux, strlen(aux));
    //    exit(-1);
    //}
    //
    //
    //addr.sin_family = AF_INET; // familia AF_INET
    //addr.sin_port = htons(port); // definimos el puerto de conexión
    //PDEBUG("INFO: Traducimos la dirección del servidor\n");
    //server = gethostbyname(dir); /** convertimos la dirección dada
    //                              * en una dirección válida para el
    //                              * equipo y la metemos en addr
    //                              */
    ////Comprobamos si ha ocurrido un error obtener el host
    //if(server == NULL){
    //    write(WRITE, "ERROR: Host inválido\n", DIM);
    //    // terminamos la ejecución del programa
    //    exit(-1);
    //}
    //// copiamos el contenido correspondiente a la dirección de server en addr
    //bcopy(server->h_addr, &(addr.sin_addr.s_addr), server->h_length);
    //
    //// realizamos la conexión al servidor
    //PDEBUG("INFO: Nos conectamos al servidor\n");
    //error = connect(sock, (struct sockaddr*) &addr, length);
    //if(error < 0){
    //    char aux[] = "ERROR: al establecer la conexion con el servidor {{connect()}}: \n";
    //    write(WRITE, aux, strlen(aux));
    //    // terminamos la ejecución del programa
    //    exit(-1);
    //}
    //
    //PDEBUG("INFO: Conexión establecida\n");
    PDEBUG("INFO: Esperando mensaje de bienvenida\n");

    memset(&auxMsj, 0, sizeof(sms));
    if(client_read(bio, &auxMsj, sizeof(sms)) <= 0){
        PDEBUG("INFO: El socket está cerrado\n");
        perror("Error al leer contenido del socket porque está cerrado\n");
        perror("El cliente se parará\n");
        exit(-1);
    }


    // atendemos la autentificación del cliente
    do{
        bzero(aux, DIM);
        sprintf(aux, "%s ~$> %s\n", auxMsj.name, auxMsj.text);
        write(WRITE, aux, DIM);
        bzero(&auxMsj.text, SMS_LEN);

        if(auxMsj.flag == MSJ){ // si es un mensaje, solo imprimimos por la salida estandar

            memset(&auxMsj, 0, sizeof(sms));
            if(client_read(bio, &auxMsj, sizeof(sms)) <= 0){
                PDEBUG("INFO: El socket está cerrado\n");
                perror("Error al leer contenido del socket porque está cerrado\n");
                perror("El cliente se parará\n");
                exit(-1);
            }
            continue;

        }

        if(auxMsj.flag == REQ_PASS){ // si es password desactivamos el echo
            echo_off();
        }
        nbytes = read(READ, &auxMsj.text, SMS_LEN);
        if(auxMsj.flag == REQ_PASS){// si es password activamos el echo
            echo_on();
        }
        auxMsj.text[nbytes - 1] = '\0'; // eliminamos el retorno de carro
        
        // nos salimos?
        if(strcmp(auxMsj.text, "-x") == 0){
            PDEBUG("EXIT: Cerrando el cliente, avisando al servidor...\n");
            auxMsj.flag = CLI_EXIT;
            client_write(bio, &auxMsj, sizeof(sms));
            PDEBUG("EXIT: Cerrando el socket\n");
            shutdown(sock, 2);
            PDEBUG("EXIT: Cerrando el cliente\n");
            exit(0);
        }else{ // es un mensaje
            strcpy(name, auxMsj.text); // hacemos una copia del nombre introducido para no perderlo
            if(auxMsj.flag == REQ_TEXT){
                auxMsj.flag = REQ_AUTH;
            }else if(auxMsj.flag == REQ_PASS){ // entonces REQ_PASS
                auxMsj.flag = CHECK_PASS;
            }else if(auxMsj.flag == REQ_ROOM){ // entonces REQ_ROOM
                auxMsj.flag = CHECK_ROOM;
            }
            client_write(bio, &auxMsj, sizeof(sms));
            memset(&auxMsj, 0, sizeof(sms));
            if(client_read(bio, &auxMsj, sizeof(sms)) <= 0){
                PDEBUG("INFO: El socket está cerrado\n");
                perror("Error al leer contenido del socket porque está cerrado\n");
                perror("El cliente se parará\n");
                exit(-1);
            }
        }
    }while(auxMsj.flag != OK);

    PDEBUG("INFO: Usuario conectado\n");
    printf("Usuario conectado...\n");
    
    fd_set desc, descCopy; // definimos un descriptor que contendrá nuestros descriptores
    //inicializamos la lista de conexiones
    FD_ZERO(&desc);
    // Inicio del bit descriptor sock con el valor de sock
    int fd;
	if(BIO_get_fd(bio, &fd) < 0){
            write(WRITE, "ERROR: crear le descriptor %s\n", DIM);
            // terminamos la ejecución del programa
            exit(-1);
	}
    FD_SET (fd, &desc);
    // Inicio del bit descriptor connList con el valor del descriptor de entrada estándar
    FD_SET (READ, &desc);

    while(1){
        // hacemos una copia de seguridad para asegurarnos de no perder los datos
        descCopy = desc;
        // ¿Hay algún socket listo para leer?
        PDEBUG("INFO: ¿Hay algún socket listo para leer?\n");
        error = select(fd + 1, &descCopy, NULL, NULL, NULL);
        //Comprobamos si ha ocurrido un error al ponernos a escuchar
        if(error < 0){
            write(WRITE, "ERROR: al realizar la selección {{select()}}: %s\n", DIM);
            // terminamos la ejecución del programa
            exit(-1);
        }

        // recorriendo los sockets para ver los que están activos
        PDEBUG("INFO: recorriendo los sockets para ver los que están activos\n");
        if(FD_ISSET(fd, &descCopy)){
            PDEBUG("INFO: Nuevo mensaje recibido\n");
            if(client_read(bio, &auxMsj, sizeof(sms)) <= 0){
                PDEBUG("INFO: El socket está cerrado\n");
                perror("Error al leer contenido del socket porque está cerrado\n");
                perror("El cliente se parará\n");
                exit(-1);
            }

            switch(auxMsj.flag){
                case OK: // mensaje de aceptacion, mismo comportamiento que msj
                case MSJ:  // mensaje recibido
                    if(serverTalk != 1 || strcmp(auxMsj.name, SERVER) == 0){
                        PDEBUG("INFO: Recibido mensaje\n");
                        sprintf(aux, "%s ~$> %s\n", auxMsj.name, auxMsj.text);
                        write(WRITE, aux, strlen(aux));
                        sync();
                    }
                    break;
                case SERV_EXIT: // el servidor se va a cerrar
                    PDEBUG("EXIT: El servidor se está cerrando, se dejará de leer\n");
                    shutdown(sock, SHUT_RDWR);
                    sprintf(aux, "%s ~$> Servidor desconectado\n", SERVER);
                    write(WRITE, aux, strlen(aux));
                    sprintf(aux, "El proceso cliente se cerrará\n");
                    write(WRITE, aux, strlen(aux));
                    exit(0);
                    break;
                default:
                    sprintf(aux, "Recibido un mensaje mal formado\n");
                    write(WRITE, aux, strlen(aux));
                    sync();
                    break;
            }
        }else if(FD_ISSET(READ, &descCopy)){
            PDEBUG("INFO: Nuevo mensaje escrito\n");

            bzero(&auxMsj.text, SMS_LEN); // inicializamos el array
            nbytes = read(READ, &auxMsj.text, SMS_LEN); // leemos de la entrada estándar
            auxMsj.text[nbytes - 1] = 0; // eliminamos el retorno de carro

            // nos salimos?
            if(strcmp(auxMsj.text, "-x") == 0 && serverTalk != 1){

                PDEBUG("EXIT: Cerrando el cliente, avisando al servidor...\n");
                auxMsj.flag = CLI_EXIT;

            }else if(strcmp(auxMsj.text, "--serv") == 0){ // queremos hablar con el servidor

                PDEBUG("SERV_ADMIN: Iniciando la comunicación directa con el servidor\n");
                sprintf(aux, "%s ~$> Iniciada conversación con el servidor\n", SERVER);
                write(WRITE, aux, strlen(aux));
                serverTalk = 1;
                continue;

            }else if(sscanf(auxMsj.text, "--mp %s", aux) == 1){ // queremos hablar con el servidor

                PDEBUG("MP: Mensaje privado detectado\n");
                strcpy(auxMsj.to, aux);
                sprintf(aux, "%s ~$> Inserte el mensaje privado\n", SERVER);
                write(WRITE, aux, strlen(aux));
                auxMsj.flag = MP;
                nbytes = read(READ, &auxMsj.text, SMS_LEN); // leemos de la entrada estándar
                auxMsj.text[nbytes - 1] = 0; // eliminamos el retorno de carro

            }else{ // es un mensaje

                if(serverTalk == 1){

                    PDEBUG("SERV_ADMIN: Enviando mensaje al servidor\n");
                    auxMsj.flag = SERV_ADMIN;

                    if(strcmp(auxMsj.text, "exit") == 0){

                        serverTalk = 0;
                        sprintf(aux, "%s ~$> Envio de mensajes de configuración terminada:\n", SERVER);
                        write(WRITE, aux, strlen(aux));
                        continue;

                    }

                }else{

                    auxMsj.flag = MSJ;

                }
            }

            strcpy(auxMsj.name, name); // hacemos una copia del nombre introducido para no perderlo
            PDEBUG("INFO: Enviando mensaje...\n");
            client_write(bio, &auxMsj, sizeof(sms));
            PDEBUG("INFO: Mensaje Enviado\n");

            // nos salimos?
            if(auxMsj.flag == CLI_EXIT){

                PDEBUG("EXIT: Cerrando el socket\n");
                shutdown(sock, SHUT_RDWR);
                PDEBUG("EXIT: Cerrando el cliente\n");
                exit(0);
                
            }
        }
    }
    
    return 0;
}
