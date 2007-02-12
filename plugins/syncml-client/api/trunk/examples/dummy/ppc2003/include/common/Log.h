/*
 * Copyright (C) 2003-2005 Funambol
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
 */

#ifndef INCL_LOG
    #define INCL_LOG

    #include <stdio.h>
    #include "fscapi.h"

    #define LOG_ERROR TEXT("ERROR")
    #define LOG_INFO  TEXT("INFO" )
    #define LOG_DEBUG TEXT("DEBUG")

    // #define LOG_NAME "sync.log"
    #define LOG_NAME TEXT("sync.txt")

    extern wchar_t logmsg[];

    class __declspec(dllexport) Log {

    private:

        FILE*   logFile;
        wchar_t logPath[256];        

        void printMessage(const wchar_t* level, const wchar_t* msg);

    public:

        Log(BOOL reset = FALSE);
        ~Log();

        void error(const wchar_t* msg);

        void info(const wchar_t* msg);

        void debug(const wchar_t* msg);

        static void setDebug(BOOL enabled);

        static BOOL isDebug();

        /*
         * Is debug enabled?
         */
        static BOOL debugEnabled;
        
        /*
        * This flag refers to INFO level and ERROR 
        */
        static BOOL infoEnabled;

        /*
        * return a the time to write into log file. If complete is true, it return 
        * the date too, else only hours, minutes, seconds and milliseconds
        */ 
        wchar_t* getCurrentTime(BOOL complete);
        
        /*
        * reset the log file if true
        */
        void reset();

        /*
        * set log path where to open the file.
        */
        void setLogPath(wchar_t* logPath);

    };

    extern Log LOG;
#endif