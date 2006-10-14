/*
 * vcalical - An vcal/ical converter 
 * Copyright (C) 2006  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2006  Christopher Stender <cstender@suse.de>
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
 */
 
#include "xml-support.h"
#include "vformat.h"
#include "xml-vcal.h"
#include <glib.h>


/* ical 2 vcal */
#define ATTR_COUNT (sizeof(rrule_attr)/sizeof(rrule_attr[0]))
#define PARAM_COUNT (sizeof(rrule_param)/sizeof(rrule_param[0]))

enum {
	FIELD_FREQ,
	FIELD_INTERVAL,
	FIELD_FREQMOD,
	FIELD_FREQMOD2,
	FIELD_COUNTUNTIL,
	NUM_OF_FIELDS
};

enum {
	TYPE_ATTR,
	TYPE_PARAM
};

struct _rrule_attr {
	const char *ical;
	const char *vcal;
	int field;
} rrule_attr[] = {
	{ "BYDAY",  " ", FIELD_FREQMOD },
	{ "BYMONTH", " ", FIELD_FREQMOD },
	{ "BYMONTHDAY", " ", FIELD_FREQMOD },
	{ "BYYEARDAY", " ", FIELD_FREQMOD },
	{ "COUNT", " #", FIELD_COUNTUNTIL },
	{ "FREQ", "", FIELD_FREQ },
	{ "INTERVAL", "", FIELD_INTERVAL },
	{ "UNTIL", " ", FIELD_COUNTUNTIL }
};

struct _rrule_param {
	const char *ical;
	const char *vcal;
} rrule_param[] = {
	{ "DAILY", "D" },
	{ "MONTHLY", "M" },
	{ "WEEKLY", "W" },
	{ "YEARLY", "YM" }
};

static int comp_attr(const void *m1, const void *m2) {
	struct _rrule_attr *mi1 = (struct _rrule_attr *) m1;
	struct _rrule_attr *mi2 = (struct _rrule_attr *) m2;
	return strcmp(mi1->ical, mi2->ical);
}
static int comp_param(const void *m1, const void *m2) {
	struct _rrule_param *mi1 = (struct _rrule_param *) m1;
	struct _rrule_param *mi2 = (struct _rrule_param *) m2;
	return strcmp(mi1->ical, mi2->ical);
}	

struct _rrule_attr *_parse_rrule_attr(const char *ical) {

	struct _rrule_attr key, *res;
	key.ical = ical;

	res = bsearch(&key, rrule_attr, ATTR_COUNT, sizeof(struct _rrule_attr), comp_attr);

	if (!res)
		return NULL;

	return res;
}

const char *_parse_rrule_param(const char *ical) {

	struct _rrule_param key, *res;
	const char *ret = NULL;

	key.ical = ical;
	
	res = bsearch(&key, rrule_param, PARAM_COUNT, sizeof(struct _rrule_param), comp_param); 

	if (!res)
		ret = ical;
	else
		ret = res->vcal;

	return ret;
}

char *_blank_field(char *field) {
	if (field)
		g_free(field);

	return g_strdup("");
}



char *_adapt_param(const char *param) {

	int i, len;
	GString *ret = g_string_new("");

	len = strlen(param);

	for (i=0; i < len; i++) {
		switch(param[i]) {
			// evil sperators like ','
			case ',':
				ret = g_string_append_c(ret, ' ');
				break;
			default:	
				ret = g_string_append_c(ret,param[i]);
		}
	}

	return g_string_free(ret, FALSE);
}

void _vcal_hook(char **icalattrs, char **vcalattrs, char **icalparams, char **vcalparams) {
	
	if (!strcmp(icalparams[FIELD_FREQ], "MONTHLY")) {
		// Workround for RRULE:MP1 1+ SU 20071003T193000
		 if(!strcmp(icalattrs[FIELD_FREQMOD], "BYDAY")) {
			char sign = '+';
			char wday[3];
			int nthday;

			g_free(vcalparams[FIELD_FREQ]);
			vcalparams[FIELD_FREQ] = g_strdup("MP");

			g_free(vcalparams[FIELD_FREQMOD]);
			
			if (strlen(icalparams[FIELD_FREQMOD]) > 3)
				sscanf(icalparams[FIELD_FREQMOD], "%c%d%c%c", &sign, &nthday, &wday[0], &wday[1]);
			else	
				sscanf(icalparams[FIELD_FREQMOD], "%d%c%c", &nthday, &wday[0], &wday[1]);

			wday[2] = '\0';

			vcalparams[FIELD_FREQMOD] = g_strdup_printf("%d%c %s", nthday, sign, wday);

		// Workaround for RRULE:MD1 .......
//		} else if (!strcmp(icalattrs[FIELD_FREQMOD], "BYMONTHDAY")) {
		} else {	
			g_free(vcalparams[FIELD_FREQ]);
			vcalparams[FIELD_FREQ] = g_strdup("MD");
		}
	}	
	
	if (!strcmp(icalparams[FIELD_FREQ], "YEARLY") && icalparams[FIELD_FREQMOD]) {
		if (!strcmp(icalattrs[FIELD_FREQMOD], "BYYEARDAY")) {
			g_free(vcalparams[FIELD_FREQ]);
			vcalparams[FIELD_FREQ] = g_strdup("YD");
		} else if ((!strcmp(icalattrs[FIELD_FREQMOD], "BYMONTH") && !strcmp(icalattrs[FIELD_FREQMOD2], "BYMONTHDAY"))
			|| (!strcmp(icalattrs[FIELD_FREQMOD2], "BYMONTH") && !strcmp(icalattrs[FIELD_FREQMOD], "BYMONTHDAY"))) {

			g_free(vcalparams[FIELD_FREQ]);
			vcalparams[FIELD_FREQ] = g_strdup("YM");

			vcalattrs[FIELD_FREQMOD] = _blank_field(vcalattrs[FIELD_FREQMOD]);
			vcalattrs[FIELD_FREQMOD2] = _blank_field(vcalattrs[FIELD_FREQMOD2]);
			vcalparams[FIELD_FREQMOD] = _blank_field(vcalparams[FIELD_FREQMOD]);
			vcalparams[FIELD_FREQMOD2] = _blank_field(vcalparams[FIELD_FREQMOD2]);
		}
	}

	// Set INTERVAL to 1 if nothing is set and BYMONTHDAY is not used
	if (icalparams[FIELD_INTERVAL] ==  NULL) {
		vcalparams[FIELD_INTERVAL] = g_strdup("1");
	}
}

char *conv_ical2vcal_rrule(const char *ical) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, ical);

        int i;
        const char *pos, *prev;
        size_t len;
	char *icalattrs[NUM_OF_FIELDS] = { NULL };
	char *vcalattrs[NUM_OF_FIELDS] = { NULL };
	char *icalparams[NUM_OF_FIELDS] = { NULL };
	char *vcalparams[NUM_OF_FIELDS] = { NULL };
	struct _rrule_attr *field_attr;
	const char *tmp = NULL;

	GString *vcal10 = g_string_new("");

        pos = prev = ical;

        // FREQ=WEEKLY;INTERVAL=1;BYDAY=TU,FR;UNTIL=20060901T182200Z
        // W1 TU FR 20060901T182200Z
	// *FREQ**INTERVAL* *FREQ-MOD* *COUNT/UNTIL*

        while ((pos = strstr(pos, "="))) {

		GString *attr = g_string_new("");
		GString *param = g_string_new("");

                len = pos - prev;

		// not equal is required ... ignoring = 
                for (i=0; i < len; i++)
			attr = g_string_append_c(attr, prev[i]);

		pos++;
		prev = pos;

                pos = strstr(pos, ";");
		if (pos == NULL)
			pos = ical + strlen(ical);

                len = pos - prev;
                for (i=0; i < len; i++)
			param = g_string_append_c(param, prev[i]);

		field_attr = _parse_rrule_attr(attr->str);
		if (field_attr == NULL)
			goto next;

		if (icalattrs[field_attr->field] && field_attr->field == FIELD_FREQMOD)
			field_attr->field += 1;

		vcalattrs[field_attr->field] = g_strdup(field_attr->vcal);
		icalattrs[field_attr->field] = g_strdup(attr->str);

		tmp = _parse_rrule_param(param->str);
		if (tmp)
			vcalparams[field_attr->field] = _adapt_param(tmp);
		else
			vcalparams[field_attr->field] = g_strdup(""); 
		icalparams[field_attr->field] = g_strdup(param->str);

		g_string_free(attr, TRUE);
		g_string_free(param, TRUE);
next:

		prev = pos + 1;

        }

	for (i=0; i < NUM_OF_FIELDS; i++) {
		if (!vcalparams[i])
			vcalparams[i] = g_strdup("");
		if (!vcalattrs[i])
			vcalattrs[i] = g_strdup("");
		if (!vcalparams[i])
			vcalparams[i] = g_strdup("");
		if (!icalattrs[i])
			icalattrs[i] = g_strdup("");
	}

	_vcal_hook(icalattrs, vcalattrs, icalparams, vcalparams);

	for (i=0; i < NUM_OF_FIELDS; i++) {
		// If no end is set append #0 - recurrence for ever
		if (i == FIELD_COUNTUNTIL && strlen(vcalparams[i]) == 0)
			vcalparams[i] = g_strdup(" #0"); 

		if (vcalattrs[i]) {
			vcal10 = g_string_append(vcal10, vcalattrs[i]);
//			printf("(%i) \"%s\"\n", i, vcalattrs[i]);
			g_free(vcalattrs[i]);
		}
		
		if (vcalparams[i]) {
			vcal10 = g_string_append(vcal10, vcalparams[i]);
//			printf("(#%i) \"%s\"\n", i, vcalparams[i]);
			g_free(vcalparams[i]);
		}

		if (icalattrs[i])
			g_free(icalattrs[i]);

		if (icalparams[i])
			g_free(icalparams[i]);

	}

	osync_trace(TRACE_EXIT, "%s: %s", __func__, vcal10->str);
	return g_string_free(vcal10, FALSE);
}

/* vcal 2 ical */
GList *conv_vcal2ical_rrule(const char *vcal) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, vcal);

	gchar** blocks = g_strsplit(vcal, " ", 256);
	int end_block;

	// variables for frequency
	int frequency_state = 0;
	char *frequency = NULL;
	char *frequency_block = NULL;

	// variables for interval
	int interval;

	// variables for duration
	int duration_number = -1;
	char *duration_timestamp = NULL;
	char *duration_block;

	// variables for frequency modifier
	char* freq_mod = NULL;	


	// count blocks
	int counter; 
	for(counter=0; blocks[counter]; counter++);

	
	// for easier access to frequency and duration define frequency_block and duration_block
	frequency_block = blocks[0];
	end_block = counter -1;
	duration_block = blocks[end_block];

		
	// get frequency: only D(1), W(2), MP(3), MD(4), YD(5) and YM(6) is allowed
	switch (*frequency_block++) {
		case 'D': frequency_state = 1; frequency = "DAILY"; break;
		case 'W': frequency_state = 2; frequency = "WEEKLY"; break;
		case 'M': frequency_state = 0;
			switch (*frequency_block++) {
				case 'P': frequency_state = 3; frequency = "MONTHLY"; break;
				case 'D': frequency_state = 4; frequency = "MONTHLY"; break;
				default:
					osync_trace(TRACE_INTERNAL, "invalid frequency M<X>");
			}
		break;	  
		case 'Y': frequency_state = 0;
			switch (*frequency_block++) {
				case 'D': frequency_state = 5; frequency = "YEARLY"; break; 
				case 'M': frequency_state = 6; frequency = "YEARLY"; break;
				default:
					osync_trace(TRACE_INTERNAL, "invalid frequency Y<X>");
			}
		break;
		default:
			osync_trace(TRACE_INTERNAL, "invalid or missing frequency");
	}


	// get interval (integer)	
	char* e;
	interval = strtol(frequency_block, &e, 3);
	if (e == frequency_block) {
		osync_trace(TRACE_INTERNAL, "interval is missing.");
	}
	if (*e != 0) {
		osync_trace(TRACE_INTERNAL, "interval is to long.");
	}


	// get frequency modifier if more than two blocks exist
	if (end_block > 1) {

		GString *fm_buffer = g_string_new("");
		int i;
	
		// for each modifier do...	
		for(i=1; i < end_block; i++) {

			int count;
			char sign;

			// if more than one modifier exist, separate them by a comma	
			if(fm_buffer->len != 0)
				g_string_append(fm_buffer, ",");

			// check frequency modifier 
			if (sscanf(blocks[i], "%d%c" , &count, &sign) == 2) {
				
				// we need to convert $COUNT- to -$COUNT  ->RFC2445
				if (sign == '-')
					count = -count;

				g_string_append_printf(fm_buffer, "%d", count);

				// if the next sign is a char, it should be a day.
				// now we need a whitespace to separate this one from
				// the previous modifier
				if (blocks[i+1] && !sscanf(blocks[i+1], "%d", &count)) {
					
					g_string_append_printf(fm_buffer, " %s", blocks[i+1]);
					i++;
					
				}
				
			} else {
				
				/* e.g. Day or LD (Last day) */
				g_string_append(fm_buffer, blocks[i]);
				
			}
		}

		freq_mod = fm_buffer->str;
		g_string_free(fm_buffer, FALSE);
	}


	/* get duration (number OR timestamp, but nothing is required) */
	if (sscanf(duration_block, "#%d", &duration_number) < 1) {
		if (strstr(duration_block,"T")) {

			duration_timestamp = osync_time_vtime2utc(duration_block);

		}
	}		

	g_strfreev(blocks);


	/* generate new RRULE: D(1), W(2), MP(3), MD(4), YD(5) and YM(6) */
	GList *new_rrule = NULL; 

	new_rrule = g_list_append(new_rrule, g_strdup_printf("FREQ=%s", frequency));
	new_rrule = g_list_append(new_rrule, g_strdup_printf("INTERVAL=%d", interval));

	if (duration_number > 0)
		new_rrule = g_list_append(new_rrule, g_strdup_printf("COUNT=%d", duration_number));
	else if (duration_timestamp != NULL)
		new_rrule = g_list_append(new_rrule, g_strdup_printf("UNTIL=%s", duration_timestamp));	


	if(freq_mod != NULL) {
		switch(frequency_state) {
			case 2:	new_rrule = g_list_append(new_rrule, g_strdup_printf("BYDAY=%s", freq_mod)); break;
			case 3:	new_rrule = g_list_append(new_rrule, g_strdup_printf("BYDAY=%s", freq_mod)); break;
			case 4:	new_rrule = g_list_append(new_rrule, g_strdup_printf("BYMONTHDAY=%s", freq_mod)); break;
			case 5:	new_rrule = g_list_append(new_rrule, g_strdup_printf("BYYEARDAY=%s", freq_mod)); break;
			case 6:	new_rrule = g_list_append(new_rrule, g_strdup_printf("BYMONTH=%s", freq_mod)); break;
			default:
				break;	
		}
	}

	osync_trace(TRACE_EXIT, "%s", __func__);

	return new_rrule;
}



