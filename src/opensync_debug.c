#include "opensync.h"
#include "opensync_internals.h"

/**
 * @defgroup OSyncDebugAPI OpenSync Debug
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * Miscanellous functions
 * 
 */
/*@{*/

/*! @brief Used for debugging
 * 
 * Used for debugging. Severity ranges from 0=Error to 5=Full Debug
 * 
 * @param subpart String to identify the subpart (and filter on it)
 * @param level The severity of the message
 * @param message The message to display
 * 
 */
void osync_debug(const char *subpart, int level, const char *message, ...)
{
		va_list arglist;
		char *buffer;
		int debug = -1;

		va_start(arglist, message);
		g_vasprintf(&buffer, message, arglist);
		
		const char *dbgstr = g_getenv("OSYNC_DEBUG");
		if (!dbgstr)
			return;
		debug = atoi(dbgstr);
		if (debug < level)
			return;
		
		switch (level) {
			case 0:
				//Error
				printf("[%s] ERROR: %s\n", subpart, buffer);
				break;
			case 1:
				// Warning
				printf("[%s] WARNING: %s\n", subpart, buffer);
				break;
		case 2:
				//Information
				printf("[%s] INFORMATION: %s\n", subpart, buffer);
				break;
		case 3:
				//debug
				printf("[%s] DEBUG: %s\n", subpart, buffer);
				break;
		case 4:
				//fulldebug
				printf("[%s] FULL DEBUG: %s\n", subpart, buffer);
				break;
		}
		va_end(arglist);
}

/*! @brief Used for printing binary data
 * 
 * Unprintable character will be printed in hex, printable are just printed
 * 
 * @param data The data to print
 * @param len The length to print
 * 
 */
void osync_print_binary(const unsigned char *data, int len)
{
  int t;
  for (t = 0; t < len; t++) {
    if (data[t] >= ' ' && data[t] <= 'z')
      printf("%c", data[t]);
    else
      printf(" %02x ", data[t]);
  }
  printf("\n");
}

/*! @brief Creates a random string
 * 
 * Creates a random string of given length or less
 * 
 * @param maxlength The maximum length of the string
 * @returns The random string
 * 
 */
char *osync_rand_str(int maxlength)
{
	char *randchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIKLMNOPQRSTUVWXYZ1234567890";
	
	int length;
	char *retchar;
	int i = 0;

	length = g_random_int_range(1, maxlength + 1);
	retchar = malloc(length * sizeof(char) + 1);
	retchar[0] = 0;

	for (i = 0; i < length; i++) {
		retchar[i] = randchars[g_random_int_range(0, strlen(randchars))];
		retchar[i + 1] = 0;
	}

	return retchar;
}

/*@}*/
