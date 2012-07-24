/* 
 * File:   database.h
 * Author: alejandro
 *
 * Created on 16 de abril de 2010, 17:58
 */

#ifndef _DATABASE_H
    #define	_DATABASE_H

    #include <sqlite3.h> 
    #include "trace.h"
    #include "sms.h"

    #define CREATE_TABLE 0
    #define INSERT 1
    #define COUNT_ROWS 2
    #define GET_ROL 3
    #define LIST_USER 4
    #define GET_LOG 5

    sqlite3* db_open(char*);
    int db_prepare(sqlite3**);
    int db_exec(sqlite3**, char*, int);
    int db_userExists(char*, sqlite3**);
    int db_checkUser(char*, char*, sqlite3**);
    int db_addUser(char*, char*, int, sqlite3**);
    int db_deleteUser(char*, sqlite3**);
    int db_listUser(int, sqlite3**);
    void db_close(sqlite3**);
    int db_addLog(sms, sqlite3**);
    int db_getLog(int, sqlite3**);
    int db_getLogPar(int, sqlite3**, char*);

#endif	/* _DATABASE_H */

