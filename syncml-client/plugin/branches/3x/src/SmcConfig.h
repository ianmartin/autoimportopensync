/*
 * Copyright (C) 2006 Michael Kolmodin
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

#ifndef  MFC_CONFIG
#define  MFC_CONFIG

#include <opensync/opensync.h>

#include <syncml-client.h>

#include <stdlib.h>
#include <string.h>
#include <glib.h>

typedef struct {
    char*     objType;
    char*     objFormat;
    char*     mimeType;

}   ObjectTypeConfig_t;


class ConfigException{
    
    public:
       const char*          why;

       ConfigException( const char* why );
};

/*
*
*  A row defining data for a given objType.
*/
class ObjectTypeConfig {

    protected:
        void read();
 
    public:
        const char* objType;
        const char* objFormat;
        const char* mimeType;

        unsigned short index;

        void readFirst();
        osync_bool hasNext();
        void readNext();
};

/**
*  The configuration read from config file.
*
*/
class SmcConfig {

    protected:
       char*         uri;               /* Gconf URI, base of gconf settings. */
       char*         syncSourceName;    /* The name of the SyncML database*/
                                        /* to sync ( "cal", "card" ... ). */
       char*         objType;           /* The type of objects we are    */
                                        /* syncing ("contact", "cal",... */
       char*         objFormat;         /* Format of objType ("vcard21",  */
                                        /*  "vevent". */
       char*         mimeType;

       /*Load the state from a xml file */
       void readConfigFile( const char* data, int size ) 
            throw( ConfigException  );

    public:
       char*    getUri();
       char*    getObjFormat();
       char*    getObjType();
       char*    getSyncSourceName();
       char*    getMimeType();


       SmcConfig( const char* data, int size ) throw ( ConfigException );

       ~SmcConfig();        
};


#endif


