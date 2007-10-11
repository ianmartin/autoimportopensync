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
// @version $Id: SyncSourceConfig.cpp,v 1.5 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"
#include "spds/common/SyncSourceConfig.h"


SyncSourceConfig::SyncSourceConfig() {
    wcscpy(name, TEXT(""));
    wcscpy(uri, TEXT(""));
    wcscpy(syncModes, TEXT(""));
    wcscpy(type, TEXT(""));
    wcscpy(sync, TEXT(""));
    last = -1;
}

wchar_t* SyncSourceConfig::getName(wchar_t* n) const {
    if (n == NULL) {
        return (wchar_t*)name;
    }

    return wcscpy(n, name);

}

void SyncSourceConfig::setName(wchar_t* n) {
    wcsncpy(name, n, DIM_SOURCE_NAME-1);
    name[DIM_SOURCE_NAME-1] = 0;
}

wchar_t* SyncSourceConfig::getURI(wchar_t* u) const {
    if (u == NULL) {
        return (wchar_t*)uri;
    }

    return  wcscpy(u, uri);
}

void SyncSourceConfig::setURI(wchar_t* u) {
    wcsncpy(uri, u, DIM_SOURCE_URI);
    uri[DIM_SOURCE_URI-1] = 0;
}


wchar_t* SyncSourceConfig::getSyncModes(wchar_t* s) const {
    if (s == NULL) {
        return (wchar_t*)syncModes;
    }

    return wcscpy(s, syncModes);
}

void SyncSourceConfig::setSyncModes(wchar_t* s) {
    wcsncpy(syncModes, s, DIM_SYNC_MODES_LIST);
    syncModes[DIM_SYNC_MODES_LIST-1] = 0;
}

wchar_t* SyncSourceConfig::getType(wchar_t* t) const {
    if (t == NULL) {
        return (wchar_t*)type;
    }

    return wcscpy(t, type);
}

void SyncSourceConfig::setType(wchar_t* t) {
    wcsncpy(type, t, DIM_MIME_TYPE);
    type[DIM_MIME_TYPE-1] = 0;
}

wchar_t* SyncSourceConfig::getSync(wchar_t* s) const {
    if (s == NULL) {
        return (wchar_t*)sync;
    }

    return wcscpy(s, sync);
}

void SyncSourceConfig::setSync(wchar_t* s) {
    wcsncpy(sync, s, DIM_SYNC_MODE);
    sync[DIM_SYNC_MODE-1] = 0;
}

wchar_t* SyncSourceConfig::getDir(wchar_t* s)  const{
    if (s == NULL) {
        return (wchar_t*)dir;
    }

    return wcscpy(s, dir);
}

void SyncSourceConfig::setDir(wchar_t* s) {
    wcsncpy(dir, s, DIM_DIR);
    dir[DIM_DIR-1] = 0;
}


void SyncSourceConfig::setLast(unsigned long t) {
    last = t;
}

unsigned long SyncSourceConfig::getLast() const {
    return (unsigned long)last;
}


// ------------------------------------------------------------- Private methods

void SyncSourceConfig::assign(const SyncSourceConfig& sc) {
    setName     (sc.getName     (NULL));
    setURI      (sc.getURI      (NULL));
    setSyncModes(sc.getSyncModes(NULL));
    setType     (sc.getType     (NULL));
    setSync     (sc.getSync     (NULL));
    setDir      (sc.getDir      (NULL));
    setLast     (sc.getLast     (    ));
}
