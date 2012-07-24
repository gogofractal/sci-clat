#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "trace.h"

int main(int argc, char** argv){
    
    if(argc == 1){
        perror("Debe indicar el nombre de la base de datos a crear\n");
        exit(-1);
    }

    PDEBUG("DB: ¿Existe el archivo?\n");
    FILE *fp = fopen(argv[1],"r");
    if( fp ) {
        PDEBUG("[Sí]\n");
        fclose(fp);
        perror("ERROR: Ya existe un archivo con ese nombre\n");
    } else {
        PDEBUG("[No]\n");
        creat(argv[1], 0);
        chmod(argv[1], 0777);
        PDEBUG("DB: Archivo Creado con permisos 777\n");
    }

    sqlite3* db;

    int error = sqlite3_open(argv[1], &db);
    if(error != SQLITE_OK){
        printf("DB: Error al crear la base de datos\n");
        exit(-1);
    }
    PDEBUG("DB: Base de datos abierta correctamente\n");

    if(db_prepare(&db) == -1){
        PDEBUG("DB: Borrando la base de datos\n");
        remove(argv[1]);
        exit(-1);
    }

    printf("Base de datos creada, el programa terminará\n");
    return 0;
}