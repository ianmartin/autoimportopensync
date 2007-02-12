#include <opensync/opensync.h>
#include <opensync/opensync_internals.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>
#include <sys/time.h>


static void usage (char *name, int ecode)
{
	fprintf (stderr, "Usage: %s\n", name);
	fprintf (stderr, "%s <Input> <Output>\n", name);
	fprintf (stderr, "[--hex]\tConvert from a plain hex format\n");
	exit (ecode);
}

osync_bool convert_hex(const char *input, unsigned int inpsize, char **output, unsigned int *outsize)
{
	int i = 0;
	GString *string = g_string_new("");
	unsigned int character = 0;
	for (i = 0; i < inpsize - 1; i = i + 2) {
		printf("Current char is %c%c\n", input[i], input[i + 1]);
		
		char tmp[3];
		tmp[0] = input[i];
		tmp[1] = input[i + 1];
		tmp[2] = 0;
		
		sscanf(tmp, " %x", &character);
		g_string_append_c(string, character);
	}
	
	*outsize = string->len;
	*output = g_string_free(string, FALSE);
	return TRUE;
}

osync_bool convert_bin(const char *input, unsigned int inpsize, char **output, unsigned int *outsize)
{
	int i = 0;
	GString *string = g_string_new("");
	
	for (i = 0; i < inpsize; i++) {
		printf("\nCurrent char is %i (%c)\n", input[i], input[i]);
		if (input[i] == '\r' || input[i] == '\n') {
			printf("Invalid input\n");
			return FALSE;
		}
		
		//char string1[2] = "";
		//char string2[3] = "";
		unsigned int character = 0;
		//int ret = ;
		printf("returned is %i\n", sscanf(input + i, " %x %*s", &character));
		printf("returned is %i\n", sscanf(input + i, " %x %*s", &character));
		if (g_pattern_match_simple(" ?? *", input + i) && sscanf(input + i, " %x %*s", &character) == 1) { 
			printf("returned is %i\n", sscanf(input + i, " %x %*s", &character));
		
		//if () { //0-9][0-9]
			
			printf("escaped chars are %.4s %i\n", input + i, sscanf(input + i, " %x %*s", &character)); 
			printf("returned is %i\n", sscanf(input + i, " %x %*s", &character));
			if (sscanf(input + i, " %x %*s", &character) != 1)
				return FALSE;
			printf("Found a escaped char %i\n", character);
			g_string_append_c(string, character);
			i+=3;
		} else {
			if (input[i] != 0) {
				printf("Appending normal char %i\n", input[i]);
				g_string_append_c(string, input[i]);
			}
		}
	}
	
	*outsize = string->len;
	*output = g_string_free(string, FALSE);
	return TRUE;
}

int main (int argc, char *argv[])
{
	OSyncError *error = NULL;
	
	osync_bool hex = FALSE;
	
	if (argc < 3)
		usage (argv[0], 1);

	int i = 1;
	for (i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--hex")) {
			hex = TRUE;
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else {
			break;
		}
	}

	char *input = argv[i];
	char *output = argv[i + 1];
	
	printf("input %s output %s\n", input, output);
	
	char *buffer = NULL;
	int size = 0;
	if (!osync_file_read(input, &buffer, &size, &error)) {
		fprintf(stderr, "%s\n", osync_error_print(&error));
		osync_error_free(&error);
		return 1;
	}
	
	char **array = g_strsplit(buffer, "\n", 0);
	g_free(buffer);
	buffer = g_strjoinv(NULL, array);
	size = strlen(buffer) + 1;
	g_strfreev(array);
	
	char *outbuffer = NULL;
	unsigned int outsize = 0;
	if (hex) {
		if (!convert_hex(buffer, size, &outbuffer, &outsize)) {
			fprintf(stderr, "Unable to convert");
			return 1;
		}
	} else {
		if (!convert_bin(buffer, size, &outbuffer, &outsize)) {
			fprintf(stderr, "Unable to convert");
			return 1;
		}
	}
	
	if (!osync_file_write(output, outbuffer, outsize, 0644, &error)) {
		fprintf(stderr, "Unable to write file %s: %s", output, osync_error_print(&error));
		osync_error_free(&error);
		return 1;
	}

	return 0;
}
