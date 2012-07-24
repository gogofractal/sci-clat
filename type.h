/* 
 * File:   type.h
 * Author: alejandro
 *
 * Created on 15 de abril de 2010, 15:29
 */

#ifndef __TYPE_H
	#define __TYPE_H

	#include <ctype.h>
	#include <string.h>
        #include <stdio.h>
        #include <stdlib.h>
        //libreria para hacer uso del md5
        #include <openssl/md5.h>

	int isNum(char[]);

        //manejo de strings
        char* ltrim(char*);
        char* rtrim(char*);
        char* trim(char*);
        int addslahses(char*, int, char*);
        void md5_sum(unsigned char*, int, int, char*);
#endif    // __TYPE_H