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
// @version $Id: Win32DeviceManager.cpp,v 1.5 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"

#include "spdm/common/Utils.h"
#include "spdm/common/ManagementNode.h"
#include "spdm/gconf/GconfDeviceManager.h"
#include "spdm/gconf/GconfManagementNode.h"

/*
 * GConf implementation of DeviceManager
 */

/*
 * Constructor
 */
GconfDeviceManager::GconfDeviceManager() : DeviceManager() {
    return;
}

/*
 * Returns the root management node for the DeviceManager.
 *
 * The ManagementNode is created with the new operator and must be
 * discarded by the caller with the operator delete.
 */
ManagementNode* GconfDeviceManager::getRootManagementNode() {
    return new GconfManagementNode(TEXT("/"), TEXT(""));
}

/*
 * Returns the management node identified by the given node pathname
 * (relative to the root management node). If the node is not found
 * NULL is returned; additional info on the error condition can be
 * retrieved calling getLastError() and getLastErrorMessage()
 *
 * The ManagementNode is created with the new operator and must be
 * discarded by the caller with the operator delete.
 */
ManagementNode* GconfDeviceManager::getManagementNode(const wchar_t* node) {
    wchar_t context[DIM_MANAGEMENT_PATH];
    wchar_t name   [DIM_MANAGEMENT_PATH];

    wmemset(context, 0, DIM_MANAGEMENT_PATH);
    wmemset(name,    0, DIM_MANAGEMENT_PATH);

    getNodeName(node, name, DIM_MANAGEMENT_PATH);
    getNodeContext(node, context, DIM_MANAGEMENT_PATH);

    return new GconfManagementNode(context, name);
}
