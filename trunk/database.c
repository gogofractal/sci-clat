#include "database.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tools.h"

#include "type.h"

#include "socket.h"
#include "sms.h"

#include "flags.h"


#define DIM 1024

static int callback(void*, int, char**, char**);

static int numRows = 0;
static int rol = 0;
static int socket = 0;

/**
 * Conecta con la base de datos llamada name, si no existe, ésta se creará y se
 * imprimirá un mensaje informativo
 */
sqlite3* db_open(char* name){
    sqlite3* db = NULL;
    int error = 0;

    PDEBUG("DB: ¿Existe el archivo?\n");
    FILE *fp = fopen(name,"r");
    if( fp ) {
        PDEBUG("[Sí]\n");
        fclose(fp);
    } else {
        PDEBUG("[No]\n");
        perror("Error, base de datos inexistente, creela antes\n");
        exit(-1);
    }

    PDEBUG("DB: Intentando abrir la base de datos\n");
    error = sqlite3_open(name, &db);
    if(error != SQLITE_OK){
        printf("DB: Error al crear la base de datos\n");
        exit(-1);
    }
    PDEBUG("DB: Base de datos abierta correctamente\n");

    return db;
}

int db_prepare(sqlite3 **db){
    char query[DIM];
    char name[DIM] = {0};
    char password[DIM] = {0};
    char aux1[DIM] = {0};
    char pass[MD5_DIGEST_LENGTH];
    int nbytes = 0;

    sprintf(query, 
            "CREATE TABLE IF NOT EXISTS users(userID INTEGER PRIMARY KEY AUTOINCREMENT, user TEXT, password TEXT, ROL INTEGER);");
    if(db_exec(db, query, CREATE_TABLE) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al crear la tabla\n");
        return -1;
    }

    printf("Nombre del administrador: \n");
    nbytes = read(0, &query, DIM);
    query[nbytes - 1] = '\0'; // eliminamos el intro
    addslahses(query, DIM, name); // escapamos las comillas simples
    printf("Contraseña del administrador: \n");
    echo_off();
    bzero(query, DIM);
    nbytes = read(0, &query, DIM);
    query[nbytes - 1] = '\0'; // eliminamos el intro
    echo_on();

    // creamos la suma
    PDEBUG("DB: Caluclando la suma MD5\n");
    MD5((unsigned char*) query, DIM, (unsigned char*)pass);
    // obtenemos la suma MD5 en caracteres ascii
    md5_sum((unsigned char*) pass, MD5_DIGEST_LENGTH, DIM, aux1);
    addslahses((char *)aux1, DIM, password);

    //creamos la query
    PDEBUG("DB: Insertando al administrador del sistema\n");
    sprintf(query,
            "INSERT INTO users (user, password, rol) VALUES ('%s', '%s', %i);",
            name, password, ADMIN);
    if(db_exec(db, query, INSERT) <0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al insertar el usuario administrador\n");
        return -1;
    }

    PDEBUG("DB: Creando la tabla del log\n");
    sprintf(query,
            "CREATE TABLE IF NOT EXISTS log(postID INTEGER PRIMARY KEY AUTOINCREMENT, author TEXT, text TEXT, time LONG);");
    if(db_exec(db, query, CREATE_TABLE) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al crear la tabla\n");
        return -1;
    }
    return 0;
}

int db_exec(sqlite3 **db, char* query, int param){
    int error = 0;
    char *errmsg = NULL;

    if(param == COUNT_ROWS){
        numRows = 0;
    }

    PDEBUG("DB: Ejecutando: \n");
    PDEBUG(query);
    PDEBUG("\n");
    error = sqlite3_exec(
                    *db,
                    query,
                    callback,
                    (void *) &param,
                    &errmsg
                );
    if(error != SQLITE_OK) {
        PDEBUG("ERROR: Error al ejecutar la query\n");
        sqlite3_free(errmsg);
        return -1;
    }

    PDEBUG(" [OK]\n");
    
    return error;
}

int db_userExists(char* name, sqlite3** db){
    char query[DIM];
    char aux[DIM];

    addslahses((char *)name, DIM, aux);
    sprintf(query, "SELECT * FROM users WHERE user='%s';", aux);
    if(db_exec(db, query, COUNT_ROWS) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al comprobar si el usuario existe\n");
        return -1;
    }
    return numRows;
}

int db_checkUser(char* name, char* password, sqlite3** db){
    char query[DIM];
    char aux[DIM];

    // creamos la suma
    PDEBUG("DB: Caluclando la suma MD5\n");
    MD5((unsigned char*) password, DIM, (unsigned char*)query);
    // obtenemos la suma MD5 en caracteres ascii
    md5_sum((unsigned char*) query, MD5_DIGEST_LENGTH, DIM, aux);
    addslahses((char *)aux, DIM, password);

    strcpy(query, name);
    addslahses((char *)query, DIM, aux);

    sprintf(query, "SELECT rol FROM users WHERE user='%s' AND password='%s';", aux, password);
    rol = 0;
    if(db_exec(db, query, GET_ROL) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al comprobar si el usuario existe\n");
        return -1;
    }
    return rol;
}

int db_addUser(char* name, char* password, int rol, sqlite3** db){
    char query[DIM];
    char aux[DIM];

    // creamos la suma
    PDEBUG("DB: Caluclando la suma MD5\n");
    MD5((unsigned char*) password, DIM, (unsigned char*)query);
    // obtenemos la suma MD5 en caracteres ascii
    md5_sum((unsigned char*) query, MD5_DIGEST_LENGTH, DIM, aux);
    addslahses((char *)aux, DIM, password);

    strcpy(query, name);
    addslahses((char *)query, DIM, aux);

    PDEBUG("DB: ¿Existe ya el usuario?\n");
    sprintf(query,
            "SELECT * FROM users WHERE user='%s';",
            aux);
    if(db_exec(db, query, COUNT_ROWS) <0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al insertar el usuario administrador\n");
        return -1;
    }
    if(numRows > 0){
        return 1;
    }

    PDEBUG("DB: Insertando el usuario\n");
    sprintf(query,
            "INSERT INTO users (user, password, rol) VALUES ('%s', '%s', %i);",
            aux, password, rol);
    if(db_exec(db, query, INSERT) <0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al insertar el usuario administrador\n");
        return -1;
    }
    return 0;
}

int db_addLog(sms msj, sqlite3** db){
    char query[DIM] = {0};
    long tm = (long)time(0);
    char aux1[DIM] = {0};
    char aux2[DIM] = {0};

    addslahses(msj.name, NAME_LEN, aux1);
    addslahses(msj.text, SMS_LEN, aux2);

    PDEBUG("DB: Insertando en el log\n");
    sprintf(query,
            "INSERT INTO LOG (author, text, time) VALUES ('%s', '%s', %ld);",
            aux1, aux2, tm);
    if(db_exec(db, query, INSERT) <0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al insertar el mensaje en el log\n");
        return -1;
    }
    return 0;
}

int db_deleteUser(char* name, sqlite3** db){
    char query[DIM];
    char aux[DIM];

    strcpy(query, name);
    addslahses((char *)query, DIM, aux);

    PDEBUG("DB: Insertando el usuario");
    sprintf(query,
            "DELETE FROM users WHERE user='%s';",
            aux);
    if(db_exec(db, query, COUNT_ROWS) <0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al borrar el usuario administrador\n");
        return -1;
    }
    return numRows;
}

int db_listUser(int socketID, sqlite3** db){
    char query[DIM];

    socket = socketID;

    sprintf(query,
            "SELECT user, rol FROM users;");
    if(db_exec(db, query, LIST_USER) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al listar el usuario administrador\n");
        return -1;
    }
    return numRows;
}

int db_getLog(int socketID, sqlite3** db){
    return db_getLogPar(socketID, db, "");
}

int db_getLogPar(int socketID, sqlite3** db, char* str){
    char query[DIM];

    socket = socketID;

    sprintf(query,
            "SELECT author, text, time FROM log %s;", str);
    if(db_exec(db, query, GET_LOG) < 0){
        sqlite3_close(*db);
        PDEBUG("DB: Error al listar el usuario administrador\n");
        return -1;
    }
    return numRows;
}

static int callback(void* arg, int argc, char** argv, char** error)
{
    int type = *((int *) arg);
    if(type == COUNT_ROWS){
        PDEBUG("DB: fila encontrada\n");
        numRows++;
    }else if(type == GET_ROL){
        rol = atoi(argv[0]);
    }else if(type == LIST_USER){
        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        sprintf(auxMsj.text, "Nombre: %s\t\tRol: %s", argv[0], argv[1]);
        write(socket, &auxMsj, sizeof(sms));
        PDEBUG("INFO: Mensaje del listado enviado al emisor\n");
    }else if(type == GET_LOG){
        sms auxMsj;
        auxMsj.flag = MSJ;
        strcpy(auxMsj.name, SERVER);
        sprintf(auxMsj.text, "Author: %s\t\tText: %s\t\tTimestamp: %s", argv[0], argv[1], argv[2]);
        write(socket, &auxMsj, sizeof(sms));
        PDEBUG("INFO: Mensaje del listado enviado al emisor\n");
    }
    return 0;
}

void db_close(sqlite3** db){
    sqlite3_close(*db);
}