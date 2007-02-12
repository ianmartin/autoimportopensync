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

 #ifndef INCL_DEVICE_MANAGER_UTIL_SPDM
    #define INCL_DEVICE_MANAGER_UTIL_SPDM

    #include "common/fscapi.h"

    /*
     * Extracts the node name from the node path
     *
     * @param node - the node path
     * @param name - the buffer that will contain the node name
     * @param size - buffer size
     */
    void getNodeName(const wchar_t* node, wchar_t* name, int size);

    /*
     * Extracts the node context from the node path
     *
     * @param node - the node path
     * @param context - the buffer that will contain the node context
     * @param size - buffer size
     */
    void getNodeContext(const wchar_t* node, wchar_t* context, int size);

    /*
     * Encode token in base64 
     *
     * @param token - the array to encode
     * @param base64 - the buffer that will contain the encoding
     * @param numBytes - byte to be encoded
     */
    void encodeBase64(void *token, char *base64, int numBytes);
    /*
     * Decode base64 in token. numBytes will contain the number of bytes decoded
     *
     * @param base64 - the array to decode
     * @param token - the buffer that will contain the encoding
     */
    void decodeBase64(char *base64, void *token, int* numBytes);


    /*
    * Function that convert char* to wchar_t*. It creates buffer with new operator
    * and has to be descarded by the caller
    * @param token - the buffer that refers to char pointer
    */
    wchar_t* char2wchar(char *token);

    /*
    * Function that convert wchar_t* to char*. It creates buffer with new operator
    * and has to be descarded by the caller
    * @param wtoken - the buffer that refers to wchar_t pointer
    */
    char* wchar2char(wchar_t* wtoken);
    void getMemoryStatus(int i);
        
#endif