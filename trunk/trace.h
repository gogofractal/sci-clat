/**
 * @File: trace.h
 * @Description: implements some tracing macros for debugging
 * @Group: 1
 * @Members:    Alejandro Escario Méndez
 *              Alejandro Fernández Alderete
 *              Carlos Martínez Rey
 */

#define DEBUG 1 // debugging
//#define DEBUG 0 // nada
//#define DEBUG 2 // syslog

#ifndef __TRACE_H
	#define __TRACE_H

	#ifndef DEBUG
            #define DEBUG 0
        #endif // DEBUG defined

	#if DEBUG == 1
            // librería para el uso de primitivas unix
            #include <unistd.h>

            /**
             * PDEBUG(x)
             * x string
             */

            #define PDEBUG(x) \
		printf("%s", x); \
		sync();
        #elif DEBUG == 2
            #include <syslog.h>

            /**
             * PDEBUG(x)
             * x string
             */
            // cat /var/log/syslog
            #define PDEBUG(x) \
		syslog(LOG_DEBUG, "%s", x);
        #else
		#define PDEBUG(x) ;
	#endif	// DEBUG == 1
#endif    // __MYTIME_H
