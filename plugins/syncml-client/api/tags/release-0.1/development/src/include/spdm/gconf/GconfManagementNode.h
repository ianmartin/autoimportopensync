/*
 * Copyright (C) 2003-2005 Funambol
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

#ifndef INCL_GCONF_MANAGEMENT_NODE
#define INCL_GCONF_MANAGEMENT_NODE

#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "spdm/common/Constants.h"
#include "spdm/common/ManagementNode.h"


/*
 * This is a concrete implementation of ManagementNode for the Gconf
 * for gnome desktop.  It uses the GConf registry services as storage for
 * configuration information. Therefore, the node context becomes the
 * registry path and a node property becomes a registry value.
 */
class GconfManagementNode : public ManagementNode  {

    private:
        gchar* mbsFullName;

    protected:
        GConfClient* gconfClient; 

    /**
    * Set error code and error message for specific function/key.
    *
    */
    void setError(int code, GError* error, gchar* key, gchar* what);

    /**
    * Set error code and error message.
    *
    */
    void setError(int code, gchar* format, ... );

    public:
        GconfManagementNode(wchar_t* context, wchar_t* name);
        GconfManagementNode(gchar* context, gchar* name);
        virtual ~GconfManagementNode();


        /*
         * Returns the value of the given property
         *
         * @param property - the property name
         * @param buf - the buffer for the value
         * @param len - the buffer size
         */
        void getPropertyValue(const wchar_t* property, wchar_t* buf, int size);


        /*
         * Sets a property value.
         *
         * @param property - the property name
         * @param value - the property value (zero terminated string)
         */
        void setPropertyValue(const wchar_t* property, wchar_t* value);

        /*
         * Returns this node's children.
         
         * The ManagementNode objects are created with the new operator and
         * must be discarded by the caller with the operator delete.
         *
         * @param children - the buffer where ManagementNode* must be stored
         * @param size - the size of the children buffer (number of ManagementNode*) in
         *               input; the number of children in output
         */
        void getChildren(ManagementNode** children, int* size);

        /*
         * Returns how many children belong to this node.
         * A negative number means an error
         *
         */
        int getChildrenCount();


        /*
         * Returns a child node of the ManagementNode.
         *
         * The child ManagementNode object is created with the new operator
         * must be discarded by the caller with the operator delete.
         *
         * @param node - subnode name
         */
        ManagementNode* getChild(const wchar_t* node);
        /*
         * delete a key into the Device Manager.
         *
         * @param key - the key to be deleted
         */
        int deleteKey(const wchar_t* keyToDelete);
        
        /*
         * test if a key exists. Return TRUE or FALSE.
         *
         * @param key - the name of the key to be checked
         */
        BOOL existsContext( void );
    };

#endif
