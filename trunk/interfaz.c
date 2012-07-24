/**
 * @File: cli5.c
 * @Description: implements the client
 * @Group: 1
 * @Members:    Alejandro Escario Méndez
 *              Alejandro Fernández Alderete
 *              Carlos Martínez Rey
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static pid_t pid;

int main(int argc, char** argv) {

    int pipe_chat[2];       // definimos el pipe que va del chat a la interfaz
    int pipe_interfaz[2];   // definimos el pipe que va de la interfaz al chat
    
    if(
        pipe(pipe_chat) == -1 || 
        pipe(pipe_interfaz) == -1
      ){
        perror("ERROR: ocurrión un error al crear los pipes\n");
        exit(-1);
    }

    if((pid = fork()) < (pid_t)0){
        perror("ERROR: ocurrió un error al crear el proceso hijo\n");
        exit(-1);
    }else if(pid == (pid_t)0){  // proceso hijo, es el que albergará la interfaz
        close(pipe_chat[1]);    // cerramos el descriptor de escritura del chat
        close(pipe_interfaz[0]); // cerramos el descriptor de lectura de la interfaz

        // cerramos los descriptores de entrada salida estandar
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        // redireccionamos la entrada y la salida
        dup2(pipe_chat[0], STDIN_FILENO);
        dup2(pipe_interfaz[1], STDOUT_FILENO);

        // cerramos los descriptores ya que no vamos a utilizarlos
        close(pipe_chat[0]);
        close(pipe_interfaz[1]);

        execl("/usr/bin/java", "java", "-jar", "chat.jar", (char*) 0);
    }else{ // proceso padre
        close(pipe_interfaz [1]);    // cerramos el descriptor de escritura de la interfaz
        close(pipe_chat[0]); // cerramos el descriptor de lectura del chat

        // cerramos los descriptores de entrada salida estandar
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        // redireccionamos la entrada y la salida
        dup2(pipe_interfaz[0], STDIN_FILENO);
        dup2(pipe_chat[1], STDOUT_FILENO);
        
        // cerramos los descriptores ya que no vamos a utilizarlos
        close(pipe_chat[0]);
        close(pipe_interfaz[1]);

        execv("./cli5", argv);
        //execl("./cli5", "cli5", (char*) 0);

    }
    return 0;
}