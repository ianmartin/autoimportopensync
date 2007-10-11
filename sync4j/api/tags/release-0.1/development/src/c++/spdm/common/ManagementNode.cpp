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

//
// @author Stefano Fornari
// @version $Id: ManagementNode.cpp,v 1.7 2005/01/19 10:22:07 magi Exp $
//

#include <stdlib.h>

#include "unixdefs.h"

#include "spdm/common/ManagementNode.h"

/** Protected default constructor, for use in derived classes. */
ManagementNode::ManagementNode() {
}

/*
 * Constructor.
 *
 * @param parent - a ManagementNode is usually under the context of a
 *                 parent node.
 * @param name - the node name
 *
 */
ManagementNode::ManagementNode(wchar_t* nodeContext, wchar_t* nodeName) {
    wcsncpy(context, nodeContext, DIM_MANAGEMENT_PATH );
    wcsncpy(name, nodeName, DIM_MANAGEMENT_PATH);
}

/** Default destructor, empty. */
ManagementNode::~ManagementNode()
{}


/*
 * Returns the full node context
 *
 */
void ManagementNode::getFullName(wchar_t* buf, int size){
    swprintf(buf, size, TEXT("%ls/%ls"), context, name);
}

