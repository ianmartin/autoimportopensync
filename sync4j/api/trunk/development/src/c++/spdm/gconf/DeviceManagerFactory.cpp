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
// @author  Michael Kolmodi
// @version $Id: DeviceManagerFactory.cpp,v 1.4 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"

#include "spdm/common/DeviceManager.h"
#include "spdm/common/DeviceManagerFactory.h"
#include "spdm/gconf/GconfDeviceManager.h"

/*
 * GConf implementation of DeviceManagerFactory
 */

/*
 * Constructor.
 */
DeviceManagerFactory::DeviceManagerFactory() {};

/*
 * Creates and returns a new DeviceManager. The DeviceManager object is create
 * with the new operator and must be deleted by the caller with the operator
 * delete
 */
DeviceManager *DeviceManagerFactory::getDeviceManager() {
    return new GconfDeviceManager();
}

