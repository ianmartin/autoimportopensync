/*
 * Copyright (C) 2003-2005 Funambol
 * Copyright (C) 2006 MIchael Kolmodin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog
 *
 * + Ported to Linux: refactored, no global filehandle, new 
 *   os-dependent functions.  /mk
 */

//
// @author Stefano Fornari
// @version $Id: Log.cpp,v 1.7 2005/03/29 13:20:24 magi Exp $
//

#include "unixdefs.h"
#include "common/Log.h"

BOOL Log::debugEnabled = false;
BOOL Log::infoEnabled  = false;

Log LOG = Log(false);

wchar_t logmsg[ DIM_LOG_MESSAGE ];

Log::Log(BOOL resetLog) {
    if (resetLog) {
        reset();
    }
    logPath[0] = 0;
}

Log::~Log() {
}

void Log::error(const wchar_t* msg) {
    if (infoEnabled == TRUE) {
        printMessage(LOG_ERROR, msg);
    }
}

void Log::info(const wchar_t* msg) {
    if (infoEnabled == TRUE) {
        printMessage(LOG_INFO, msg);
    }
}

void Log::debug(const wchar_t* msg) {
    if (debugEnabled == TRUE) {
        printMessage(LOG_DEBUG, msg);
    }
}

void Log::info( void )
{
    info( logmsg );
}

void Log::debug( void )
{
    debug( logmsg );
}

void Log::error( void )
{
    error( logmsg );
}


void Log::setMessage(  const wchar_t* message ... )  
{
    wchar_t* message_arg;

    va_list ap;
    va_start( ap, message );
    vswprintf( logmsg, DIM_LOG_MESSAGE, message, ap );    
    va_end( ap );

}

void Log::setDebug(BOOL enabled) {
    debugEnabled = enabled;
}

BOOL Log::isDebug() {
    return debugEnabled;
}

void Log::reset() 
{
    printMessage( L"INFO", L" # SyncClient API Native Log\n", "w" );
}


void Log::setLogPath(wchar_t* configLogPath) {
    
    if (configLogPath != NULL) {
        swprintf(logPath, DIM_LOG_MESSAGE, TEXT("%ls/"), configLogPath); 
    } else {
        swprintf(logPath,  DIM_LOG_MESSAGE, TEXT("")); 
    }
}


void Log::printMessage(const wchar_t* level, 
                       const wchar_t* msg,
                       const char*    accessMode  ) {       
    
    wchar_t* currentTime = NULL;
    char     currentLogPath [256];

    currentTime = getCurrentTime(false);
    snprintf(currentLogPath, DIM_LOG_MESSAGE, "%ls%ls", 
             logPath, LOG_NAME);
    
    FILE* logFile  = fopen(currentLogPath, accessMode );          
    assert( logFile != NULL );
    fprintf(logFile,  "\n%ls [%ls] - %ls", currentTime, level, msg );
    fclose(logFile);

    delete[] currentTime;
}


/*
* return a the time to write into log file. If complete is true, it return 
* the date too, else only hours, minutes, seconds and milliseconds
*/ 
wchar_t* Log::getCurrentTime(BOOL complete) 
{

    struct timeval now_timeval;
    time_t now_sec;
    struct tm now;
    char   buff[64];

    gettimeofday( &now_timeval, NULL );
    localtime_r( &(now_timeval.tv_sec), &now );

    if (complete) {
        char fmtComplete[] = "%04d-%02d-%02d %02d:%02d:%02d:%03d";
        sprintf(buff, fmtComplete, now.tm_year + 1900, now.tm_mon + 1, 
                      now.tm_mday,
                      now.tm_hour, now.tm_min, now.tm_sec, 
                      now_timeval.tv_usec / 1000);
    } 
    else {
        char fmt[]  = "%02d:%02d:%02d:%03d";
        sprintf(buff, fmt, now.tm_hour, now.tm_min, now.tm_sec, 
                      now_timeval.tv_usec / 1000  );
    }
    return new_mbsrtowcs( buff );
}

