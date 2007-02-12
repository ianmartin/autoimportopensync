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

#ifndef INCL_MANAGEMENT_NODE
    #define INCL_MANAGEMENT_NODE

    #include "common/fscapi.h"
    #include "spdm/common/Constants.h"

    /*
     * This class represents a management node, so that a configuration
     * object under the device manager control.
     * This is an abstract class that defines the interface of platform
     * specific concrete implementations.
     *
     * See the design documents for more information.
     */
    class __declspec(dllexport) ManagementNode {

    protected:
        wchar_t name   [DIM_MANAGEMENT_PATH];
        wchar_t context[DIM_MANAGEMENT_PATH];


    public:
        /*
         * Constructor.
         *
         * @param parent - a ManagementNode is usually under the context of a
         *                 parent node.
         * @param name - the node name
         *
         */
        ManagementNode(wchar_t* context, wchar_t* name);


        // ----------------------------------------------------- Virtual methods

        /*
         * Returns the value of the given property
         *
         * @param property - the property name
         * @param buf - the buffer for the value
         * @param len - the buffer size
         */
        virtual void getPropertyValue(const wchar_t* property, wchar_t* buf, int size) = 0;


        /*
         * Sets a property value.
         *
         * @param property - the property name
         * @param value - the property value (zero terminated string)
         */
        virtual void setPropertyValue(const wchar_t* property, wchar_t* value) = 0;

        /*
         * Returns this node's children.
         *
         * The ManagementNode objects are created with the new operator and
         * must be discarded by the caller with the operator delete.
         *
         * @param children - the buffer where ManagementNode* must be stored
         * @param size - the size of the children buffer (number of ManagementNode*) in
         *               input; the number of children in output
         *
         */
        virtual void getChildren(ManagementNode** children, int* size) = 0;

        /*
         * Returns how many children belong to this node.
         *
         */
        virtual int getChildrenCount() = 0;


        /*
         * Returns a child node of the ManagementNode.
         *
         * The child ManagementNode object is created with the new operator
         * must be discarded by the caller with the operator delete.
         *
         * @param node - subnode name
         */
        virtual ManagementNode* getChild(const wchar_t* node) = 0;

        /*
         * delete a key into the Device Manager.
         *
         * @param key - the key to be deleted
         */
        virtual int deleteKey(const wchar_t* keyToDelete) = 0;


        /*
         * test if a key exists. Return TRUE or FALSE.
         *
         * @param key - the name of the key to be checked
         */
        virtual BOOL existsContext() = 0; 

        // ------------------------------------------------------ Public methods


        /*
         * Returns the full node name
         *
         */
        void getFullName(wchar_t* buf, int size);


    };

#endif