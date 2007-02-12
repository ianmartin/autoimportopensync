/*
 * Copyright (C) 2003-2005 Funambol
 * Copyright (C) 2005 Michael Koldmodin
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
 *
 * Changelog
 *     + Ported to Linux, #ifdef LINUX parts /mk
 */
//
// @author Stefano Fornari
// @version $Id: SPDMUtils.cpp,v 1.7 2005/01/19 10:22:07 magi Exp $
//

#include "unixdefs.h"

#include "spdm/common/Constants.h" 
#include "spdm/common/Utils.h"
#include "spdm/common/base64.h" 
#include "common/Log.h"

#ifdef LINUX

/**
*
* Maps a character string to a wide-character (Unicode) string. The inout string
* is supposed to be in platform charset, normalle UTF-8 on Unix/Linux.
* mapped by this function is not necessarily from a multibyte character set.
*
*/
int MultiByteToWideChar( unsigned int  CodePage,    // code page, must be CP_ACP
                         DWORD         dwFlags,     // character-type options
                         LPCSTR        lpMultiByteStr, // string to map
                         size_t        cbMultiByte, // number of bytes in string
                         LPWSTR        lpWideCharStr,  // wide-character buffer
                         size_t        cchWideChar   )    // size of buffer
{
    char* charset;

    assert( CodePage == CP_ACP  );
    mbsrtowcs( lpWideCharStr, &lpMultiByteStr, cbMultiByte, NULL);

}

int WideCharToMultiByte( UINT    CodePage,           // code page, must be CP_ACP
                         DWORD   dwFlags,            // performance and mapping flags, not used.
                         LPCWSTR  lpWideCharStr,      // wide-character string
                         size_t  ccWideChar,        // number of chars in string
                         LPSTR   lpMultiByteStr,     // buffer for new string
                         size_t  cbMultiByte,        // size of buffer
                         LPCSTR  lpDefaultChar,      // default for unmappable chars, not used
                         LPBOOL  lpUsedDefaultChar)  // set when default char used, not used.
{

    size_t written;
    int total = 0;

    if( cbMultiByte == 0 )
        cbMultiByte = MAXINT;
    while( ccWideChar > 0 && cbMultiByte > 0 ){
        written = wcrtomb( lpMultiByteStr, *lpWideCharStr, NULL);
        if( written < 0 ){
            setErrorMsg( ERR_INVALID_SEQUENCE, TEXT("Cannot convert unicode string to UTF-8") );
            return( 0 ); 
        }
        else{
            lpMultiByteStr += written;
            ccWideChar -= 1;
            cbMultiByte -= 1;
            total += written;
        }
    }
    return total; 
           
}

#endif //LINUX


 /*
 * Extracts the node name from the node path
 *
 * @param node - the node path
 * @param name - the buffer that will contain the node name
 * @param size - buffer size
 */
void getNodeName(const wchar_t* node, wchar_t* name, int size) {
    wchar_t* p;

    p = wcsrchr(node, L'/');

    if (p == NULL) {
        wcsncpy(name, node, size-1);
        return;
    }

    wcsncpy(name, p+1, size);
}



/*
 * Extracts the node context from the node path
 *
 * @param node - the node path
 * @param context - the buffer that will contain the node context
 * @param size - buffer size
 */
void getNodeContext(const wchar_t* node, wchar_t* context, int size) {
    wchar_t* p;

    p = wcsrchr(node, L'/');

    if (p == NULL) {
        *context = L'\0';
        return;
    }

    wcsncpy( context, node, (p - node  < size ? p - node : size ) );
}

/*
 * Encode token in base64 
 *
 * @param token - the array to encode
 * @param base64 - the buffer that will contain the encoding
 * @param numBytes - byte to be encoded
 */

void encodeBase64(void *token, char *base64, int numBytes) {
                    
    int rett = b64_encode(base64, token, numBytes);

    base64[rett] = 0;
    
     
}
/*
 * Decode base64 in token. numBytes will contain the number of bytes decoded
 *
 * @param base64 - the array to decode
 * @param token - the buffer that will contain the encoding
 */

void decodeBase64(char *base64, void *token, int* numBytes) {
        
    int lung = strlen(base64);    
        
    int rett = b64_decode(token, base64);
    
    *numBytes = rett;
}


/*
* Function that convert char* to wchar_t*. It creates buffer with new operator
* and has to be descarded by the caller
* @param token - the buffer that refers to char pointer
*/

wchar_t* char2wchar(char *token) {
    
	int len = 0;
    wchar_t *ret = NULL;	
    if (token == NULL) 
        goto finally;
    
    len = strlen(token);
    
    if (len == 0)
        goto finally;

	ret = new wchar_t[len + 1];
	MultiByteToWideChar (CP_ACP, 0, token, len, ret, len);
	ret[len] = 0;	

finally:
    return ret;
}

/*
* Function that convert wchar_t* to char*. It creates buffer with new operator
* and has to be descarded by the caller
* @param wtoken - the buffer that refers to wchar_t pointer
*/

char* wchar2char(wchar_t* wtoken) {

    int len = 0;
	BOOL usedDefaultChar;
    char* ret = NULL;
    if (wtoken == NULL)
        goto finally;
    
    len = wcslen(wtoken);    
    if (len == 0)
        goto finally;

    ret = new char[len + 1];
		
	WideCharToMultiByte (CP_ACP, 0, wtoken, len, ret, len, "?", &usedDefaultChar);		
	ret[len] = 0;
    

finally:
    return ret;

}

#ifndef LINUX
void getMemoryStatus(int i) {
    
    MEMORYSTATUS        memInfo;
    wchar_t             szBuf[512];
    
    // memInfo.dwLength = sizeof(memInfo);
    
    GlobalMemoryStatus(&memInfo);
    wsprintf(szBuf, TEXT("Debug n. % i ---> Total RAM: %d bytes  -- Free: %d -- Used: %d"), i, memInfo.dwTotalPhys, memInfo.dwAvailPhys, memInfo.dwTotalPhys - memInfo.dwAvailPhys);

    LOG.debug(szBuf);
}

#else  // LINUX

void getMemoryStatus(int i) {

    char                line[ 128 ];
    wchar_t             szBuf[512];
    char*               strtok_ptr;
    char*               keyword;
    char*               value;
    unsigned int        totalMemory;
    unsigned int        freeMemory;

    FILE* meminfo = fopen( "/proc/meminfo", "r" );
    if( meminfo < 0 )
        wcscpy( szBuf, TEXT("Cannot open /proc/meminfo") );
    else{
        while( fgets( line, sizeof( line ), meminfo ) != NULL  ){
            keyword = strtok_r( line, ": \t", &strtok_ptr );
            value = strtok_r( NULL, " \t", &strtok_ptr );
            if( strcmp( keyword, "MemTotal" ) == 0 )
                totalMemory = atoi( value );
            else if(  strcmp( keyword, "MemFree" ) == 0 )
                freeMemory = atoi( value );
        }
        swprintf(szBuf, sizeof( szBuf ), 
                TEXT("Debug n. %d  ---> Total RAM: %d bytes  -- Free: %d -- Used: %d"), 
                 i, totalMemory, freeMemory, totalMemory - freeMemory);    
        fclose( meminfo );
    }
    LOG.debug(szBuf);
}

#endif  //LINUX
