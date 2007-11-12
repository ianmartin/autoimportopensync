/*
 * vconvert - A tool for converting vcards, vevents, vtodos and vnotes
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Chris Toshok (toshok@ximian.com)
 * Author: Armin Bauer (armin.bauer@opensync.org)
 * Author: Christopher Stender (cstender@suse.de)
 *
 */


#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-support.h>
#include <opensync/opensync-data.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>

static void usage (char *name, int ecode)
{
	fprintf (stderr, "Usage: %s <file> <switches>\n", name);
	fprintf (stderr, "--out <file>\tStore the output in this file (No output to stdout)\n");
	fprintf (stderr, "--to-vcard21\tConvert to vcard 2.1\n");
	fprintf (stderr, "--to-vcard30\tConvert to vcard 3.0\n");
	fprintf (stderr, "--to-vevent10\tConvert to vevent 1.0\n");
	fprintf (stderr, "--to-vevent20\tConvert to vevent 2.0\n");
	fprintf (stderr, "--to-vnote11\tConvert to vnote 1.1\n");
	fprintf (stderr, "--to-vjournal\tConvert to vjournal\n");
	fprintf (stderr, "--to-vtodo10\tConvert to vtodo 1.0\n");
	fprintf (stderr, "--to-vtodo20\tConvert to vtodo 2.0\n");
	fprintf (stderr, "--to-xmlformat\tConvert to xmlformat\n");
	exit (ecode);
}

typedef enum conv_detection {
	TARGET_AUTO = 0,
	TARGET_VCARD_21 = 1,
	TARGET_VCARD_30 = 2,
	TARGET_VEVENT_10 = 3,
	TARGET_VEVENT_20 = 4,
	TARGET_VNOTE_11 = 5,
	TARGET_VJOURNAL = 6,
	TARGET_VTODO_10 = 7,
	TARGET_VTODO_20 = 8,
	TARGET_XMLFORMAT = 9
} conv_detection;

OSyncObjFormat *conv_run_detection(OSyncFormatEnv *env, OSyncChange *change, conv_detection type)
{
	OSyncObjFormat *sourceformat = NULL;
	OSyncError *error = NULL;
	OSyncData *data = NULL;
	OSyncObjFormat *targetformat = NULL;

	data = osync_change_get_data(change);

	if (!(sourceformat = osync_format_env_detect_objformat_full(env, data, &error))) {
		fprintf(stderr, "Unable to detect file format\n");
		goto out;
	}

	osync_data_set_objformat(data, sourceformat);

	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard21")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VCARD_30:
			targetformat = osync_format_env_find_objformat(env, "vcard30");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-contact");
			break;
		default:
			fprintf(stderr, "Unable to convert vcard21 into this format. Supported formats: xmlformat, vcard30\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard30")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VCARD_21:
			targetformat = osync_format_env_find_objformat(env, "vcard21");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-contact");
			break;
		default:
			fprintf(stderr, "Unable to convert vcard30 into this format. Supported formats: xmlformat, vcard21\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent10")) {
		switch (type) {
                case TARGET_AUTO:
                case TARGET_VEVENT_20:
                        targetformat = osync_format_env_find_objformat(env, "vevent20");
                        break;
                case TARGET_XMLFORMAT:
                        targetformat = osync_format_env_find_objformat(env, "xmlformat-event");
                        break;
                default:
                        fprintf(stderr, "Unable to convert vevent10 into this format. Supported formats: xmlformat, vevent20\n");
                }
                goto out;
        }

	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent20")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VEVENT_10:
			targetformat = osync_format_env_find_objformat(env, "vevent10");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-event");
			break;
		default:
			fprintf(stderr, "Unable to convert vevent20 into this format. Supported formats: xmlformat, vevent10\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vnote11")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VJOURNAL:
			targetformat = osync_format_env_find_objformat(env, "vjournal");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-note");
			break;
		default:
			fprintf(stderr, "Unable to convert vnote11 into this format. Supported formats: xmlformat, vjournal\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vjournal")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VNOTE_11:
			targetformat = osync_format_env_find_objformat(env, "vnote11");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-note");
			break;
		default:
			fprintf(stderr, "Unable to convert vjournal into this format. Supported formats: xmlformat, vnote11\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo10")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VTODO_20:
			targetformat = osync_format_env_find_objformat(env, "vtodo20");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-todo");
			break;
		default:
			fprintf(stderr, "Unable to convert vtodo10 into this format. Supported formats: xmlformat, vtodo20\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo20")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VTODO_10:
			targetformat = osync_format_env_find_objformat(env, "vtodo10");
			break;
		case TARGET_XMLFORMAT:
			targetformat = osync_format_env_find_objformat(env, "xmlformat-todo");
			break;
		default:
			fprintf(stderr, "Unable to convert vtodo20 into this format. Supported formats: xmlformat, vtodo10\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "xmlformat-contact-doc")) {
		switch (type) {
		case TARGET_VCARD_21:
			targetformat = osync_format_env_find_objformat(env, "vcard21");
			break;
		case TARGET_AUTO:
		case TARGET_VCARD_30:
			targetformat = osync_format_env_find_objformat(env, "vcard30");
			break;
		default:
			fprintf(stderr, "Unable to convert xmlformat-contact-doc into this format. Supported formats: vcard21, vcard30\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "xmlformat-event-doc")) {
		switch (type) {
		case TARGET_VEVENT_10:
			targetformat = osync_format_env_find_objformat(env, "vevent10");
			break;
		case TARGET_AUTO:
		case TARGET_VEVENT_20:
			targetformat = osync_format_env_find_objformat(env, "vevent20");
			break;
		default:
			fprintf(stderr, "Unable to convert xmlformat-event-doc into this format. Supported formats: vevent10, vevent20\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "xmlformat-note-doc")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VNOTE_11:
			targetformat = osync_format_env_find_objformat(env, "vnote11");
			break;
		default:
			fprintf(stderr, "Unable to convert xmlformat-note-doc into this format. Supported formats: vnote11\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "xmlformat-journal-doc")) {
		switch (type) {
		case TARGET_AUTO:
		case TARGET_VJOURNAL:
			targetformat = osync_format_env_find_objformat(env, "vjournal");
			break;
		default:
			fprintf(stderr, "Unable to convert xmlformat-journal-doc into this format. Supported formats: vjournal\n");
		}
		goto out;
	}

	if (!strcmp(osync_objformat_get_name(sourceformat), "xmlformat-todo-doc")) {
		switch (type) {
		case TARGET_VTODO_10:
			targetformat = osync_format_env_find_objformat(env, "vtodo10");
			break;
		case TARGET_AUTO:
		case TARGET_VTODO_20:
			targetformat = osync_format_env_find_objformat(env, "vtodo20");
			break;
		default:
			fprintf(stderr, "Unable to convert xmlformat-todo-doc into this format. Supported formats: vtodo10, vtodo20\n");
		}
		goto out;
	}

	fprintf(stderr, "Cannot convert objtype %s. Unable to find a converter\n", osync_objformat_get_name(sourceformat));
out:
	return targetformat;
}

int main (int argc, char *argv[])
{
	if (argc < 2)
		usage (argv[0], 1);

	char *filename = argv[1];
	char *output = NULL;
	conv_detection type = TARGET_AUTO;
	OSyncObjFormat *targetformat = NULL;

	int i;
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--to-vcard21")) {
			type = TARGET_VCARD_21;
		} else if (!strcmp (arg, "--to-vcard30")) {
			type = TARGET_VCARD_30;
		} else if (!strcmp (arg, "--to-vevent10")) {
			type = TARGET_VEVENT_10;
		} else if (!strcmp (arg, "--to-vevent20")) {
			type = TARGET_VEVENT_20;
		} else if (!strcmp (arg, "--to-vnote11")) {
			type = TARGET_VNOTE_11;
		} else if (!strcmp (arg, "--to-vjournal")) {
			type = TARGET_VJOURNAL;
		} else if (!strcmp (arg, "--to-vtodo10")) {
			type = TARGET_VTODO_10;
		} else if (!strcmp (arg, "--to-vtodo20")) {
			type = TARGET_VTODO_20;
		} else if (!strcmp (arg, "--to-xmlformat")) {
			type = TARGET_XMLFORMAT;
		} else if (!strcmp (arg, "--out")) {
			output = argv[i + 1];
			i++;
			if (!output)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--")) {
			break;
		} else if (arg[0] == '-') {
			usage (argv[0], 1);
		} else {
			usage (argv[0], 1);
		}
	}

	OSyncError *error = NULL;

	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	if (!format_env) {
		fprintf(stderr, "Unable to create format environment\n");
		goto error;
	}

	if (!osync_format_env_load_plugins(format_env, NULL, &error)) {
		fprintf(stderr, "Unable to load plugins\n");
		goto error;
	}

	char *buffer;
	unsigned int size;
	if (!osync_file_read(filename, &buffer, &size, &error)) {
		fprintf(stderr, "Unable to open file\n");
		goto error;
	}

	// osync_file_read returns a \0 terminated string, but plugins
	// have to report the right size, not strlen
	size++;

	OSyncData *data = NULL;
	OSyncChange *change = osync_change_new(&error);
	if (!change)
		goto error;

	osync_change_set_uid(change, filename);

	// just setup a dummyformat to complete data for osync_data_new
	OSyncObjFormat *dummyformat = osync_objformat_new("plain", "data", &error);

	if (!(data = osync_data_new(buffer, size, dummyformat, &error))) {
		goto error;
	}

	osync_change_set_data(change, data);

	osync_objformat_unref(dummyformat);	

	// detect source and target xmlformat
	targetformat = conv_run_detection(format_env, change, type);
	if (targetformat == NULL)
		exit (1);
	OSyncObjFormat *sourceformat = osync_data_get_objformat(data);

	// find converter path
	OSyncFormatConverterPath *path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);

	// convert data
	if (!osync_format_env_convert(format_env, path, data, &error)) {
		fprintf(stderr, "Unable to convert data\n");
		goto error;
	}

	char *print = osync_data_get_printable(data);

	if (output) {
		if (!osync_file_write(output, print, strlen(print), 0644, &error)) {
			fprintf(stderr, "Unable to write file %s", output);
			goto error;
		}
	} else {
		printf("%s", print);
	}

	g_free(print);

	osync_data_unref(data);
	osync_change_unref(change);
	osync_converter_path_unref(path);
	osync_format_env_free(format_env);

	return 0;

error:
	if (error)
		fprintf(stderr, "%s\n", osync_error_print(&error));

	osync_error_unref(&error);
	return 1;
}
