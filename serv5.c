/**
 * @File: serv5.c
 * @Description: implements the server
 * @Group: 1
 * @Members:    Alejandro Escario Méndez
 *              Alejandro Fernández Alderete
 *              Carlos Martínez Rey
 */
/*
 * expulsion de usuarios
 * mensaje broadcast
 */
#include <stdio.h>
#include <stdlib.h>
// librería para manejo de strings
#include <string.h>
// librería para el uso de los sockets y las correspondientes constantes
#include <sys/socket.h>
// librería para el uso de la constante IPPROTO_TCP, in_addr, ...
#include <netinet/in.h>
// librería para el uso de primitivas unix
#include <unistd.h>
// librería que nos permite hacer uso de la variable errno
#include <errno.h>
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
// incuimos la librería de tipos de datos
#include "type.h"
// incluimos la librería que va a controlar la base de datos
#include "database.h"
// incluimos la librería que va a convertir el proceso en un demonio
#include "tools.h"
// incluimos las librerias de ssl
#include <openssl/ssl.h>
#include <openssl/err.h>

//tamaño de la cola de peticiones
#define Q_DIM 5 // es el valor recomendado por defecto

#define DIM 1024 

int main(int argc, char** argv){
    
    int sock = 0; // declaración del socket e inicializado a 0
    int error = 0; /** declaramos una variable que nos servirá para detectar
                    * errores
                    */
    socklen_t length = (socklen_t) sizeof (struct sockaddr_in); // tamaño del paquete
    struct sockaddr_in addr; // definimos el contenedor de la dirección
    unsigned int port = 5678; /** creamos la variable que identifica el puerto
                               * de conexión, siendo el puerto por defecto 5678
                               */
    int connPos = 0; // primera posición libre en el array de conexiones
    int connTam = 10; // tamaño actual del array de conexiones
    int connGrow = 10; // factor de crecimiento del array
    user* conn = NULL; // array de conexiones con los clientes
    room rooms[DIM];
    user auxConn;   // conexion auxiliar
    sms auxMsj;
    fd_set connList, connListCopy; // definimos un descriptor que contendrá nuestros sockets
    int nbytes = 0; // contador de bytes leidos y escritos
    int dbName = 0; // variable que nos permitirá configurar el nombre de la base de datos
    sqlite3* db = NULL; // descriptor de la base de datos

    char cert[DIM] = "cert"; // nombre del certificado del servidor
    char pkey[DIM] = "pkey"; // nombre del archivo con la clave privada

    // <editor-fold defaultstate="collapsed" desc="Interpretado de parámetros de entrada">
    //analizamos los parámetros de entrada
    int i = 0;
    for(; i < argc; i++){
        if(strcmp(argv[i], "-p") == 0){ // leemos el puerto
            if(argc <= i + 1 || isNum(argv[i+1]) == 0){
                perror("Se esperaba un número después de -p");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó un puerto\n");
                i++;
                port = atoi(argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-ls") == 0){ // leemos el tamaño inicial de la lista
            if(argc <= i + 1 || isNum(argv[i+1]) == 0){
                perror("Se esperaba un número después de -ls");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó un tamaño inicial\n");
                i++;
                connTam = atoi(argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-lg") == 0){ // leemos el factor de creciemiento de la lista de conexiones
            if(argc <= i + 1 || isNum(argv[i+1]) == 0){
                perror("Se esperaba un número después de -lg\n");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó un crecimiento\n");
                i++;
                connGrow = atoi(argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-db") == 0){ // leemos el nombre de la base de datos que queremos utilizar
            if(argc <= i + 1){
                perror("Se esperaba una cadena depués de -db\n");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó un crecimiento\n");
                i++;
                dbName = i;
            }
            continue;
        }else if(strcmp(argv[i], "-cert") == 0){ // leemos el nombre del archivo del certificado
            if(argc <= i + 1){
                perror("Se esperaba una cadena depués de -cert\n");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó un certificado\n");
                i++;
                strcpy(cert, argv[i]);
            }
            continue;
        }else if(strcmp(argv[i], "-pkey") == 0){ // leemos el nombre del archivo de que contiene la clave privada
            if(argc <= i + 1){
                perror("Se esperaba una cadena depués de -pkey\n");
                exit(-1);
            }else{
                PDEBUG("ARGS: Se detectó una clave privada\n");
                i++;
                strcpy(pkey, argv[i]);
            }
            continue;
        }
    }
    //</editor-fold>

    db = db_open(
            (dbName == 0) ? "chat.db" : argv[dbName]
            );

    PDEBUG("INFO: Convertimos el proceso en un demonio\n");
    //make_daemon();


    /*******************************SSL****************************************/
    PDEBUG("INFO: Inicializando la libreria SSL\n");
    SSL_library_init();
    PDEBUG("INFO: Cargamos los algoritmos SSL y los mensajes de error\n");
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    PDEBUG("INFO: Seleccionamos SSLv2, SSLv3 y TLSv1\n");
    SSL_METHOD *method;
    method = SSLv23_server_method();
    PDEBUG("INFO: Creamos el nuevo contexto\n");
    SSL_CTX *ctx;
    ctx = SSL_CTX_new(method);
    if(ctx == NULL) { // error
        ERR_print_errors_fp(stderr);
        _exit(-1);
    }

    PDEBUG("INFO: Comprobando el certificado\n");
    if ( SSL_CTX_use_certificate_chain_file(ctx, cert) <= 0) {
        ERR_print_errors_fp(stderr);
        _exit(-1);
    }

    PDEBUG("INFO: Comprobando la clav eprivada\n");
    if ( SSL_CTX_use_PrivateKey_file(ctx, pkey, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        _exit(-1);
    }

    PDEBUG("INFO: Comprobando que las claves pueden trabajar juntas\n");
    if ( !SSL_CTX_check_private_key(ctx) ) {
        fprintf(stderr, "Clave privada incorrecta.\n");
        _exit(-1);
    }

    /*******************************SSL****************************************/

    //Creamos el socket
    PDEBUG("INFO: Creando el socket\n");
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //Comprobamos si ha ocurrido un error al crear el socket
    if(sock < 0){
        write(2, strcat("ERROR: creación del socket {{socket()}}: %s\n", strerror(errno)), DIM);
        // terminamos la ejecución del programa
        exit(-1);
    }

    PDEBUG("INFO: Estableciendo el puerto, origenes,...\n");
    addr.sin_family = AF_INET; // familia AF_INET
    addr.sin_port = htons(port); // definimos el puerto de conexión
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // permitimos conexion de cualquiera

    /* hacemos este "apaño" porque según hemos leido, http://www.wlug.org.nz/EADDRINUSE
     * hay un timeout para liberar el socket
     */
    unsigned int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1) {
        write(2, "ERROR: al permitir la reutiización del puerto {{setsockopt()}}\n", DIM);
        exit(-1);
    }

    // le asignamos una dirección al socket
    PDEBUG("INFO: Asignando una dirección al socket\n");
    error = bind(sock, (struct sockaddr *)&addr, length);
    //Comprobamos si ha ocurrido un error al hacer el bind
    if(error < 0){
        write(2, strcat("ERROR: {{bind()}}: %s\n", strerror(errno)), DIM);
        // terminamos la ejecución del programa
        exit(-1);
    }

    //Ponemos el servidor a escuchar para buscar nuevas conexiones
    PDEBUG("INFO: Comenzamos la escucha de l programa\n");
    error = listen(sock, Q_DIM);
    //Comprobamos si ha ocurrido un error al ponernos a escuchar
    if(error < 0){
        write(2, strcat("ERROR: al iniciar la escucha{{listen()}}: %s\n", strerror(errno)), DIM);
        // terminamos la ejecución del programa
        exit(-1);
    }

    // realizamos la asignación inicial de memoria
    PDEBUG("INFO: Realizando asignación inicial de memoria, tamaño inicial 10\n");
    connTam = 10;
    conn = malloc(connTam * sizeof(user));
    // rellenamos el array con -1
    memset(conn, 0, connTam * sizeof(user));

    //inicializamos la lista de conexiones
    FD_ZERO(&connList);
    // Inicio del bit descriptor connList con el valor de sock
    FD_SET (sock, &connList);
    
    PDEBUG("INFO: Creamos la sala de chat general\n");
    bzero(rooms, DIM * sizeof(room));
    strcpy(rooms[0].name, "general");

    // <editor-fold defaultstate="collapsed" desc="Bucle de escucha">
    //comenzamos a analizar conexiones
    PDEBUG("INFO: Comenzamos a analizar los sockets\n");
    while(1){
        // hacemos una copia de seguridad para asegurarnos de no perder los datos
        connListCopy = connList;
        // ¿Hay algún socket listo para leer?
        PDEBUG("INFO: ¿Hay algún socket listo para leer?\n");
        error = select(connTam + 1, &connListCopy, NULL, NULL, NULL);
        //Comprobamos si ha ocurrido un error al ponernos a escuchar
        if(error < 0){
            write(2, strcat("ERROR: al realizar la selección {{select()}}: %s\n"
                       , strerror(errno)), DIM);
            // terminamos la ejecución del programa
            exit(-1);
        }

        // recorriendo los sockets para ver los que están activos
        PDEBUG("INFO: recorriendo los sockets para ver los que están activos\n");
        int i = 0; // definimos un índice
        for (; i <= connTam; i++){
            // este socket está preparado para leer los datos
            if(FD_ISSET(i, &connListCopy)){
                // vemos si el socket preparado para leer es el de aceptar peticiones
                if(i == sock){
                    PDEBUG("INFO: Nuevo cliente detectado, comprobando...\n");
                    auxConn.sock = accept(sock, (struct sockaddr *) &addr, &length);
                    if(auxConn.sock < 0){
                        write(2, "ERROR: al realizar la aceptación {{accept()}}: %s\n"
                                   , *strerror(errno));
                        // terminamos la ejecución del programa
                        exit(-1);
                    }

                    /************************SSL*******************************/
                    PDEBUG("INFO: Creando conexion ssl\n");
                    PDEBUG("INFO: Creando conexion SSL\n");
                    auxConn.ssl = SSL_new(ctx);
                    PDEBUG("INFO: Asignando la conexión a SSL\n");
                    SSL_set_fd(auxConn.ssl, auxConn.sock);
                    PDEBUG("INFO: Aceptando la conexión SSL\n");
                    error = SSL_accept(auxConn.ssl);
                    if(error < 0){
                        ERR_print_errors_fp(stderr);
                        exit(-1);
                    }
                    /************************SSL*******************************/

                    PDEBUG("INFO: Conexión establecida, autenticando...\n");

                    memset(&auxMsj, 0, sizeof(auxMsj)); // incializamos la estructura

                    PDEBUG("INFO: Solicitando autenticación\n");
                    strcpy(auxMsj.text, "Usuario: "); // establecemos el texto que queremos que se muestre
                    auxMsj.flag = REQ_TEXT;  // le indicamos que requerimos una respuesta con texto
                    strcpy(auxMsj.name, SERVER); // nos identificamos como el servidor
                    SSL_write(auxConn.ssl, &auxMsj, sizeof(sms)); // enviamos la información
                    
                    // metemos los datos de la conexión en nuestro array de conexiones
                    strcpy((*(conn + connPos)).name, auxMsj.text);
                    (*(conn + connPos)).sock = auxConn.sock;
                    (*(conn + connPos)).ssl = auxConn.ssl;
                    (*(conn + connPos)).prov = PROV;

                    // Añadimos el socket a nuestra lista
                    PDEBUG("INFO: Insertando socket en la lista de monitoreo\n");
                    FD_SET (auxConn.sock, &connList);

                    // como la peticion se ha aceptado incrementamos el contador de conexiones
                    PDEBUG("INFO: Cálculo del nuevo offset\n");
                    nextPos(conn, &connPos, &connTam, connGrow);
                }else{ // si no, es un cliente ya registrado

                    PDEBUG("DATA: Nuevo mensaje detectado\n");
                    nbytes = SSL_read((*(conn+searchConn(conn, connTam, i))).ssl, &auxMsj, sizeof(sms));
                    if(nbytes > 0){ // si hemos leido más d eun byte...
                        
                        switch(auxMsj.flag){

                            case CLI_EXIT: // desconexión del cliente
                                closeConn(conn, &connPos, connTam, i, &connList, db);
                                break;
                            case SERV_ADMIN: // parámetros que ha de ejecutr el servidor
                                execParams(conn, connTam, auxMsj.text, i, sock, db, rooms, DIM);
                                break;
                            case MSJ:  // mensaje
                                multicast(conn, &connTam, auxMsj, i, db,
                                            (*(conn+searchConn(conn, connTam, i))).room);
                                break;
                            case REQ_AUTH: // vamos a leer el nombre de usuario
                                auth(conn, &connTam, i, auxMsj, db, rooms, DIM);
                                break;
                            case CHECK_ROOM: // vamos a leer el nombre de usuario
                                roomCheckIn(conn, &connTam, i, auxMsj, db, rooms, DIM);
                                break;
                            case CHECK_PASS:
                                authPassword(conn, &connTam, i, auxMsj, db, rooms, DIM);
                                break;
                            case MP:
                                mp(conn, &connTam, auxMsj, i, db);
                                break;
                            default:
                                write(2, "ERROR: Recibido un mensaje mal formado\n", 39);
                                break;
                                
                        }

                    }else{ // hemos detectado una desconexión por el cerrado de la conexión

                        closeConn(conn, &connPos, connTam, i, &connList, db);

                    }
                }
            }
        }
    }//</editor-fold>

    return 0;
}
