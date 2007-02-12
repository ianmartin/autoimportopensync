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
// @version $Id: SyncSourceConfig.h,v 1.5 2005/01/19 10:22:08 magi Exp $
//

#ifndef INCL_SYNC_SOURCE_CONFIG
#define INCL_SYNC_SOURCE_CONFIG

#include "spds/common/Constants.h"

    class __declspec(dllexport) SyncSourceConfig {

    private:

        wchar_t name     [DIM_SOURCE_NAME    ];
        wchar_t uri      [DIM_SOURCE_URI     ];
        wchar_t syncModes[DIM_SYNC_MODES_LIST];
        wchar_t type     [DIM_MIME_TYPE      ];
        wchar_t sync     [DIM_SYNC_MODE      ];
        wchar_t dir      [DIM_DIR            ];

        unsigned long last;

        void assign(const SyncSourceConfig& sc);

    public:

        /*
         * Constructs a new SyncSourceConfig object
         */
        SyncSourceConfig();

        /*
         * Returns the SyncSource name. If name is null, the internal buffer is
         * returned, otherwise the value is copied into the given buffer (that
         * must be DIM_SOURCE_NAME big).
         *
         * @param n the buffer were the name will be copied into (if not null)
         *
         */
        wchar_t* getName(wchar_t* n) const;

        /*
         * Sets the SyncSource name
         *
         * @param n the new name
         */
        void setName(wchar_t* n);


        /*
         * Returns the SyncSource URI (used in SyncML addressing). If uri is
         * null, the internal buffer is returned, otherwise the value is copied
         * into the given buffer (that must be DIM_SOURCE_URI big).
         *
         * @param u the buffer were the uri will be copied into (if not null)
         */
        wchar_t* getURI(wchar_t* u) const;

        /*
         * Sets the SyncSource URI (used in SyncML addressing).
         *
         * @param u the new uri
         */
        void setURI(wchar_t* u);

        /*
         * Returns a comma separated list of the possible syncModes for the
         * SyncSource. If syncModes is null, the internal buffer is returned,
         * otherwise the value is copied into the given buffer (that must be
         * DIM_SYNC_MODES_LIST big).
         *
         * @param s the buffer were the sync modes list will be copied into
         *          (if not null)
         */

        wchar_t* getSyncModes(wchar_t* s) const;

        /*
         * Sets the available syncModes for the SyncSource as comma separated
         * values. Each value must be one of:
         *  . none
         *  . slow
         *  . two-way
         *  . one-way
         *  . refresh
         *
         * @param s the new list
         */
        void setSyncModes(wchar_t* s);

        /*
         * Returns the mime type of the items handled by the sync source.
         * If type is null, the internal buffer is returned, otherwise the
         * value is copied into the given buffer (that must be DIM_SYNC_MIME_TYPE
         * big).
         *
         * @param t the buffer were the mime type will be copied into (if not null)
         */
        wchar_t* getType(wchar_t* t) const;

        /*
         * Sets the mime type of the items handled by the sync source.
         *
         * @param t the mime type
         */
        void setType(wchar_t* t);

        /*
         * Sets the default syncMode as one of the strings listed in setSyncModes.
         *
         * @param s the sync mode
         */
        wchar_t* getSync(wchar_t* s) const;

        /*
         * Returns the default syncMode as one of the strings above. If type is
         * null, the internal buffer is returned, otherwise the value is copied
         * into the given buffer (that must be DIM_SYNC_MODE big).
         *
         * @param t the buffer were the sync mode will be copied into (if not null)
         */
        void setSync(wchar_t* s);

        /*
         * Sets the last sync timestamp
         *
         * @param timestamp the last sync timestamp
         */
        void setLast(unsigned long timestamp);

        /*
         * Returns the last sync timestamp
         */
        unsigned long getLast() const;

        /*
         * Returns the content of the dir property. This is path directory
         * used to sync files, notes...
         * If t is null, the internal buffer is returned, otherwise the
         * value is copied into the given buffer (that must be DIM_SYNC_DIR
         * big).
         *
         * @param t the buffer were the dir will be copied into (if not null)
         */
        wchar_t* getDir(wchar_t* t) const;

        /*
         * Sets the dir of the items handled by the sync source.
         *
         * @param t the dir value
         */
        void setDir(wchar_t* t);


        /*
         * Copies the given sync source config into this one.
         *
         * @param sc - the sync source config to copy
         */
        SyncSourceConfig& operator= (const SyncSourceConfig& sc) { assign(sc); return *this; }
    };

#endif

