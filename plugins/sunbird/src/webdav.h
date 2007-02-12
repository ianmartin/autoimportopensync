#define WEBDAV_SUCCESS 0 /* Request succeeded */
#define WEBDAV_ERROR_INIT 1 /* Could not init 'neon' library */
#define WEBDAV_ERROR_CONNECT 2 /* Could not connect to server */
#define WEBDAV_ERROR_RESSOURCE 3 /* Cannot read or write file */
#define WEBDAV_ERROR_FILE_NOT_FOUND 4 /* Local file not found */
#define WEBDAV_ERROR_INVALID_PARAMETER 5 /* Parameter not in valid range */
#define WEBDAV_ERROR_OUT_OF_MEMORY 6 /* Out of memory */

/* Download a file from a server */
int webdav_download(char* filename, char* url, char* username, char* password);

/* Upload a file to a server */
int webdav_upload(char* filename, char* url, char* username, char* password);
