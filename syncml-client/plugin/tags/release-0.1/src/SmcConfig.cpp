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
 
#include <opensync/opensync.h>
#include <libxml/parser.h>
#include <glib.h>

#include "SmcConfig.h"

const ObjectTypeConfig_t ObjectConfiguration[] =
{
    { "contact", "vcard21",  "text/x-vcard" },
    { "event",   "vevent10", "text/x-vcalendar" },
    { "todo",    "vtodo10",  "text/x-vcalendar" },
    { "note",    "vnote11",  "text/plain" },
};

const unsigned short NrOfObjectTypes
    = sizeof( ObjectConfiguration ) / sizeof( ObjectTypeConfig_t);

void ObjectTypeConfig::readFirst()
{
    index = 0;
    read();
};

osync_bool ObjectTypeConfig::hasNext()
{
    return (index + 1) < NrOfObjectTypes;
};

void ObjectTypeConfig::readNext()
{
    assert( hasNext() );
    index += 1;
    read();
};

void ObjectTypeConfig::read()
{
    objType   = ObjectConfiguration[ index ].objType;
    objFormat = ObjectConfiguration[ index ].objFormat;
    mimeType  = ObjectConfiguration[ index ].mimeType;
};

char*    SmcConfig::getUri()              { return uri; }
char*    SmcConfig::getObjFormat()        { return objFormat; }
char*    SmcConfig::getObjType()          { return objType; }
char*    SmcConfig::getSyncSourceName()   { return syncSourceName; }
char*    SmcConfig::getMimeType()         { return mimeType; }

static osync_bool xmlStringMatch( const xmlChar* arg1, const char* arg2 )
{
    return xmlStrcmp( arg1, ( const xmlChar*) arg2 ) == 0;
}

/*Load the state from a xml file */
void SmcConfig::readConfigFile( const char* data, int size ) 
    throw ( ConfigException )
{
    osync_trace(TRACE_ENTRY, "%s(%p, %i)", __func__, data, size);
    xmlDocPtr doc;
    xmlNodePtr cur;

    doc = xmlParseMemory(data, size);
    if (!doc) 
       throw ConfigException(  "Unable to parse settings" );

    cur = xmlDocGetRootElement(doc);
    if (!cur){
        xmlFreeDoc(doc);  
        throw ConfigException( "Unable to get root element of the settings" );
    } 

    if ( ! xmlStringMatch(cur->name, "config") ) {
        xmlFreeDoc(doc);
        throw ConfigException( "Unable to parse settings" );
    }

    for( cur = cur->xmlChildrenNode;  cur != NULL; cur = cur->next ){
        char *str = (char*)xmlNodeGetContent(cur);
        if (str) {
            if ( xmlStringMatch( cur->name, "gconf-uri")) {
                uri = g_strdup(str);
            }
            else if ( xmlStringMatch( cur->name, "object-type" )){
                objType = g_strdup(str);
            }
            else if ( xmlStringMatch( cur->name, "sync-source-name" )){
                syncSourceName = g_strdup(str);
            }
            xmlFree(str);
        }
    }
    xmlFreeDoc(doc);
        
    if (  uri == NULL )
        throw ConfigException( "gconf-uri not set" );
    if( objType == NULL )
        throw ConfigException(  "object-type not set"  );        
    if( syncSourceName == NULL )
        throw ConfigException( "sync-source-name not set" );        

    osync_trace(TRACE_EXIT, "%s", __func__);
}

SmcConfig::SmcConfig( const char* data, int size ) 
    throw( ConfigException )
{
    // Set defaults
    uri      = NULL;
    objType  = NULL;

    readConfigFile( data, size );

    mimeType = objFormat = NULL;
    for( int i = 0;  i < NrOfObjectTypes; i += 1 ){
       if( strcmp( objType,  ObjectConfiguration[i].objType ) == 0 ) {
           objFormat =  ObjectConfiguration[i].objFormat;
           mimeType  =  ObjectConfiguration[i].mimeType;
           break;
       }
    }
}

SmcConfig::~SmcConfig() 
{
    g_free( uri );
    g_free( syncSourceName );
    g_free( objType );    
}

ConfigException::ConfigException( const char* whyArg )
    : why( whyArg )    
{}


