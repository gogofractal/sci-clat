
// http://www.sbin.org/doc/unix-faq/programmer/faq_4.html
#include "tools.h"

#include <unistd.h>
// para STDIN,...
#include <sys/types.h>
// para umask
#include <sys/stat.h>

static struct termios stored_settings;

void echo_off(void)
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= (~ECHO);
    tcsetattr(0,TCSANOW,&new_settings);
    return;
}

void echo_on(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}

void make_daemon(void){
    pid_t pid, sid;
    pid = fork();
    if (pid < (pid_t)0) { 	// error
            perror("Se produjo un error al crear el proceso hijo\n");
            exit(-1);
    }else if(pid == (pid_t)0){	// proceso hijo
        // convertimos el proceso en lider de sesión, de manera que no dependa de la consola
        sid = setsid();
        if (sid < 0){
            exit(-1);
        }

        // establecemos el umask para los ficheros temporales
        umask(027);

        // establecemos la carpeta de ejecución del demonio
        if ((chdir("./")) < 0){
            exit(-1);
        }

        // cerramos los descriptores de fichero innecesarios
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }else{	// proceso padre
        exit(0);
    }
}

