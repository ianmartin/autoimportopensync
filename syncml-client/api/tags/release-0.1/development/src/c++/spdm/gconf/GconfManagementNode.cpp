/*
 * Copyright (C) 2005 Michael Kolmodin
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

//
// @author Michael Kolmodin
// @version $Id:$
//

/*
 * CGonf implementation of ManagementNode
 */
#include "unixdefs.h"

#include "common/fscapi.h"
#include "common/Constants.h"
#include "common/Log.h"
#include "spdm/common/Utils.h"
#include "spdm/common/Constants.h"
#include "spdm/common/ManagementNode.h"
#include "spdm/gconf/GconfManagementNode.h"

static gboolean g_type_init_done = FALSE;


GconfManagementNode::GconfManagementNode(wchar_t* context, wchar_t* name) 
   : ManagementNode(context, name) 
{
    mbsFullName = g_strdup_printf( "%ls/%ls", context, name );
    if( ! g_type_init_done ){
        g_type_init();
        g_type_init_done = TRUE;
    }
    gconfClient = gconf_client_get_default();  
    LOG.setMessage( L"Created wide char node, context: %ls, name: %ls", 
                    context, name );
    LOG.debug();

}

GconfManagementNode::GconfManagementNode( gchar* context, gchar* name ) 
{
    mbstowcs( ManagementNode::name, name, DIM_MANAGEMENT_PATH );
    mbstowcs( ManagementNode::context, context, DIM_MANAGEMENT_PATH );
    mbsFullName = g_strdup_printf( "%s/%s", context, name );
    gconfClient = gconf_client_get_default();  
    LOG.setMessage( L"Created  char node, context: %s, name: %s", 
                    context, name );
    LOG.debug();
}

GconfManagementNode::~GconfManagementNode()
{
    g_object_unref( gconfClient );
    g_free( mbsFullName );
}

/**
* Set error code and error message
*
*/

void GconfManagementNode::setError(int       code, 
                                   GError*   error, 
                                   gchar*    what, 
                                   gchar*    key)
{
    lastErrorCode = code;
    swprintf( lastErrorMsg, DIM_ERROR_MESSAGE, 
              L"Error: %s, context: %s, key:%s, doing: %s",
              error->message, 
              mbsFullName, 
              what, 
              key );
    LOG.setMessage( lastErrorMsg );
    LOG.info();
}

void GconfManagementNode::setError(int code, gchar* format, ... )
{

    wchar_t* wcsFormat = new_mbsrtowcs( format );
    va_list ap;
    va_start( ap, format );
    vswprintf( lastErrorMsg, DIM_ERROR_MESSAGE, wcsFormat, ap );
    va_end( ap );

    lastErrorCode = code;

    LOG.setMessage( lastErrorMsg );
    LOG.info();
    delete[] wcsFormat;
}


/*
 * Returns the value of the given property. Returns emptru string
 * on errors.
 *
 * @param property - the property name
 * @param buf - the buffer for the value
 * @param len - the buffer size
 *
 * FIXME: The interface has no means to communicate an error to caller!
 */
void GconfManagementNode::getPropertyValue(const wchar_t* property, 
                                           wchar_t*       buf, 
                                           int            size) 
{
   
    assert( property != 0 && wcslen( property) > 0 );
    assert( buf != 0 ); 

    gchar* key      = g_strdup_printf( "%s/%ls", mbsFullName, property );

    GError* error   = NULL;
    gchar* value    = gconf_client_get_string( gconfClient, key, &error );

    if( error == NULL ){
        if( value != 0 ){
            mbstowcs( buf,  value, size );
            free( value );
        }
        else{
            *buf = L'\0';
            LOG.setMessage( TEXT("Looking up unset property: %s") , 
                            key );
            LOG.info();
        }
    }
    else{
        setError( ERR_INVALID_KEY, error, "getPropertyValue", key );
        g_error_free( error );
    }

    LOG.setMessage( L"Read property: %s, value: %ls", key, buf );
    LOG.debug();
    g_free( key );
}

/*
 * Sets a property value.
 *
 * @param property - the property name
 * @param value - the property value (zero terminated string)
 */
void GconfManagementNode::setPropertyValue(const wchar_t* property, 
                                           wchar_t*       value) 
{

    gchar* key       = g_strdup_printf( "%s/%ls", mbsFullName, property );
    gchar* mbsValue = new_wcstombs( value );

    GError* error  = NULL;
    if( ! gconf_client_set_string( gconfClient, key, mbsValue, &error ) ){
        setError( ERR_INVALID_KEY, error, "getPropertyValue", key );
        g_error_free( error );
    }
    delete[] mbsValue; 
    g_free( key );
}


/**
 * Delete a key in the Device Manager.
 *
 * @param key - the key to be deleted
 */
int GconfManagementNode::deleteKey(const wchar_t* keyToDelete) 
{

    swprintf( lastErrorMsg, DIM_ERROR_MESSAGE, 
               L"Attempt to delete key %ls", keyToDelete );
    lastErrorCode = ERR_NOT_IMPLEMENTED;
    return( -1 );
}

/**
 *
 * Test if the context exists. Return TRUE or FALSE.
 *
 */
BOOL GconfManagementNode::existsContext( void ) 
{
    return gconf_client_dir_exists( gconfClient, mbsFullName, NULL );
}

/*
 * Returns this node's children.
 *
 * The ManagementNode objects are created with the new operator and
 * must be discarded by the caller with the operator delete.
 *
 * @param children - the buffer where ManagementNode* must be stored
 * @param size - the size of the children buffer (number of ManagementNode*) in
 *               input; the number of children in output
 * FIXME: delete list after use
 */
void GconfManagementNode::getChildren(ManagementNode** children, int* size) 
{
    gchar* fullName[ DIM_MANAGEMENT_PATH ];

    if ( ! this->existsContext() ) {
        setError( ERR_INVALID_CONTEXT,
                  "Invalid context path: %s", mbsFullName );
        *size = -1;
    }

    GSList* childs = gconf_client_all_dirs( gconfClient, mbsFullName, NULL );

    int maxSize  = *size;
    *size = 0;
    gchar* path;
    gchar* name; 
    while( ( path = (gchar*) g_slist_nth_data(childs, *size ) ) != NULL  
             && *size < maxSize )
    {
                name = strrchr( path, '/' );
                if( name == NULL )
                    name = path;
                else
                    name++; 
                children[ *size ] = 
                     new GconfManagementNode( mbsFullName, name );
                g_free( path );
                *size += 1;
    }
    g_slist_free( childs );
}

/*
 * Returns how many children belong to this node.
 * A negative number means an error
 * FIXME: delete list after use
 *
 */
int GconfManagementNode::getChildrenCount() 
{

    if ( ! this->existsContext() ) {
        setError( ERR_INVALID_CONTEXT, 
                  "Invalid context path: %s", mbsFullName );
        return( -1 );
    }

    GSList* childs = gconf_client_all_dirs( gconfClient, mbsFullName, NULL );
    int count = 0;
    gpointer data;
    while( ( data = g_slist_nth_data(childs, count ) ) != NULL ){
        count += 1;
        g_free( data );
    }
    g_slist_free( childs );
    return count;
}

/*
 * Returns a child node of the ManagementNode.
 *
 * The child ManagementNode object is created with the new operator
 * must be discarded by the caller with the operator delete.
 *
 * @param node - subnode name
 */
ManagementNode* GconfManagementNode::getChild( const wchar_t* node) 
{
    LOG.setMessage( L"Creating child, name: %ls, nodeName: %s", 
                    node, mbsFullName );
    LOG.debug();

    gchar*  mbsNode = new_wcstombs( node );

    ManagementNode* managementNode = 
        new GconfManagementNode(mbsFullName, mbsNode );

    delete mbsNode;
    return( managementNode );
}


