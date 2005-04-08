/*
 * Copyright (C) 2003 Ximian, Inc. 2005 Armin Bauer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Chris Toshok (toshok@ximian.com)
 * Author: Armin Bauer (armin.bauer@opensync.org)
 * 
 */

#include "vformat.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static size_t base64_encode_step(unsigned char *in, size_t len, gboolean break_lines, unsigned char *out, int *state, int *save);
static size_t base64_decode_step(unsigned char *in, size_t len, unsigned char *out, int *state, unsigned int *save);
size_t base64_decode_simple (char *data, size_t len);
char  *base64_encode_simple (const char *data, size_t len);

size_t quoted_decode_simple (char *data, size_t len);
char *quoted_encode_simple (const unsigned char *string, int len);

time_t vformat_time_to_unix(const char *inptime)
{	
	char *date = NULL;
	char *time = NULL;
	char *ftime = NULL;
	if ((ftime = g_strrstr(inptime, "T"))) {
		
		date = g_strndup(inptime, ftime - inptime);
		if (ftime[3] == ':')
			time = g_strndup(ftime + 1, 8);
		else
			time = g_strndup(ftime + 1, 6);
	} else {
		date = g_strdup(inptime);
	}
	
	struct tm btime;
	memset(&btime, 0, sizeof(struct tm));

	if (strlen(date) == 10) {
		btime.tm_year = date[0] * 1000 + date[1] * 100 + date[2] * 10 + date[3] - '0' * 1111 - 1900;
		btime.tm_mon = date[5] * 10 + date[6] - '0' * 11 - 1;
		btime.tm_mday = date[8] * 10 + date[9] - '0' * 11;
	} else {
		btime.tm_year = date[0] * 1000 + date[1] * 100 + date[2] * 10 + date[3] - '0' * 1111- 1900;
		btime.tm_mon = date[4] * 10 + date[5] - '0' * 11 - 1;
		btime.tm_mday = date[6] * 10 + date[7] - '0' * 11;
	}
		
	if (time && strlen(time) == 8) {
		//Time
		btime.tm_hour = time[0] * 10 + time[1] - '0' * 11;
		btime.tm_min = time[3] * 10 + time[4] - '0' * 11;
		btime.tm_sec = time[6] * 10 + time[7] - '0' * 11;
	} else if (time && strlen(time) == 6) {
		btime.tm_hour = time[0] * 10 + time[1] - '0' * 11;
		btime.tm_min = time[2] * 10 + time[3] - '0' * 11;
		btime.tm_sec = time[4] * 10 + time[5] - '0' * 11;
	}

	time_t utime = mktime(&btime);
	return utime;
}

static char *_fold_lines (char *buf)
{
	GString *str = g_string_new ("");
	char *p = buf;
	char *next, *next2;

	/* we're pretty liberal with line folding here.  We handle
	   lines folded with \r\n<WS>... and \n\r<WS>... and
	   \n<WS>... We also turn single \r's and \n's not followed by
	   WS into \r\n's. */
	while (*p) {
		if (*p == '\r' || *p == '\n') {
			next = g_utf8_next_char (p);
			if (*next == '\n' || *next == '\r') {
				next2 = g_utf8_next_char (next);
				if (*next2 == ' ' || *next2 == '\t') {
					p = g_utf8_next_char (next2);
				}
				else {
					str = g_string_append (str, CRLF);
					p = g_utf8_next_char (next);
				}
			}
			else if (*next == ' ' || *next == '\t') {
				p = g_utf8_next_char (next);
			}
			else {
				str = g_string_append (str, CRLF);
				p = g_utf8_next_char (p);
			}
		}
		else {
			str = g_string_append_unichar (str, g_utf8_get_char (p));
			p = g_utf8_next_char (p);
		}
	}

	g_free (buf);

	return g_string_free (str, FALSE);
}

/* skip forward until we hit the CRLF, or \0 */
static void _skip_to_next_line (char **p)
{
	char *lp;
	lp = *p;

	while (*lp != '\r' && *lp != '\0')
		lp = g_utf8_next_char (lp);

	if (*lp == '\r') {
		lp = g_utf8_next_char (lp); /* \n */
		lp = g_utf8_next_char (lp); /* start of the next line */
	}

	*p = lp;
}

/* skip forward until we hit a character in @s, CRLF, or \0.  leave *p
   pointing at the character that causes us to stop */
static void _skip_until (char **p, char *s)
{
	char *lp;

	lp = *p;
	
	while (*lp != '\r' && *lp != '\0') {
		gboolean s_matches = FALSE;
		char *ls;
		for (ls = s; *ls; ls = g_utf8_next_char (ls)) {
			if (g_utf8_get_char (ls) == g_utf8_get_char (lp)) {
				s_matches = TRUE;
				break;
			}
		}

		if (s_matches)
			break;
		lp++;
	}

	*p = lp;
}

static void _read_attribute_value (VFormatAttribute *attr, char **p, gboolean quoted_printable)
{
	char *lp = *p;
	GString *str;

	/* read in the value */
	str = g_string_new ("");
	while (*lp != '\r' && *lp != '\0') {
		if (*lp == '=' && quoted_printable) {
			char a, b;
			if ((a = *(++lp)) == '\0') break;
			if ((b = *(++lp)) == '\0') break;
			if (a == '\r' && b == '\n') {
				/* it was a = at the end of the line,
				 * just ignore this and continue
				 * parsing on the next line.  yay for
				 * 2 kinds of line folding
				 */
			}
			else if (isalnum(a) && isalnum (b)) {
				char c;

				a = tolower (a);
				b = tolower (b);

				c = (((a>='a'?a-'a'+10:a-'0')&0x0f) << 4)
					| ((b>='a'?b-'a'+10:b-'0')&0x0f);

				str = g_string_append_c (str, c);
			}
			/* silently consume malformed input, and
			   continue parsing */
			lp++;
		}
		else if (*lp == '\\') {
			/* convert back to the non-escaped version of
			   the characters */
			lp = g_utf8_next_char(lp);
			if (*lp == '\0') {
				str = g_string_append_c (str, '\\');
				break;
			}
			switch (*lp) {
				case 'n': str = g_string_append_c (str, '\n'); break;
				case 'r': str = g_string_append_c (str, '\r'); break;
				case ';': str = g_string_append_c (str, ';'); break;
				case ',':
					if (!strcmp (attr->name, "CATEGORIES")) {
						//We need to handle categories here to work
						//aroung a bug in evo2
						vformat_attribute_add_value (attr, str->str);
						g_string_assign (str, "");
					} else
						str = g_string_append_c (str, ',');
					break;
				case '\\': str = g_string_append_c (str, '\\'); break;
				case '"': str = g_string_append_c (str, '"'); break;
				default:
					g_warning ("invalid escape, passing it through. escaped char was %i", *lp);
					str = g_string_append_c (str, '\\');
					str = g_string_append_unichar (str, g_utf8_get_char(lp));
					break;
			}
			lp = g_utf8_next_char(lp);
		}
		else if ((*lp == ';') ||
			 (*lp == ',' && !strcmp (attr->name, "CATEGORIES"))) {
			vformat_attribute_add_value (attr, str->str);
			g_string_assign (str, "");
			lp = g_utf8_next_char(lp);
		}
		else {
			str = g_string_append_unichar (str, g_utf8_get_char (lp));
			lp = g_utf8_next_char(lp);
		}
	}
	if (str) {
		vformat_attribute_add_value (attr, str->str);
		g_string_free (str, TRUE);
	}

	if (*lp == '\r') {
		lp = g_utf8_next_char (lp); /* \n */
		lp = g_utf8_next_char (lp); /* start of the next line */
	}

	*p = lp;
}

static void _read_attribute_params(VFormatAttribute *attr, char **p, gboolean *quoted_printable)
{
	char *lp = *p;
	GString *str;
	VFormatParam *param = NULL;
	gboolean in_quote = FALSE;
	str = g_string_new ("");
	
	while (*lp != '\0') {
		if (*lp == '"') {
			in_quote = !in_quote;
			lp = g_utf8_next_char (lp);
		}
		else if (in_quote || g_unichar_isalnum (g_utf8_get_char (lp)) || *lp == '-' || *lp == '_' || *lp == '/' || *lp == '.' || *lp == ' ') {
			str = g_string_append_unichar (str, g_utf8_get_char (lp));
			lp = g_utf8_next_char (lp);
		}
		/* accumulate until we hit the '=' or ';'.  If we hit
		 * a '=' the string contains the parameter name.  if
		 * we hit a ';' the string contains the parameter
		 * value and the name is either ENCODING (if value ==
		 * QUOTED-PRINTABLE) or TYPE (in any other case.)
		 */
		else if (*lp == '=') {
			if (str->len > 0) {
				param = vformat_attribute_param_new (str->str);
				g_string_assign (str, "");
				lp = g_utf8_next_char (lp);
			}
			else {
				_skip_until (&lp, ":;");
				if (*lp == '\r') {
					lp = g_utf8_next_char (lp); /* \n */
					lp = g_utf8_next_char (lp); /* start of the next line */
					break;
				}
				else if (*lp == ';')
					lp = g_utf8_next_char (lp);
			}
		}
		else if (*lp == ';' || *lp == ':' || *lp == ',') {
			gboolean colon = (*lp == ':');
			gboolean comma = (*lp == ',');

			if (param) {
				if (str->len > 0) {
					vformat_attribute_param_add_value (param, str->str);
					g_string_assign (str, "");
					if (!colon)
						lp = g_utf8_next_char (lp);
				}
				else {
					/* we've got a parameter of the form:
					 * PARAM=(.*,)?[:;]
					 * so what we do depends on if there are already values
					 * for the parameter.  If there are, we just finish
					 * this parameter and skip past the offending character
					 * (unless it's the ':'). If there aren't values, we free
					 * the parameter then skip past the character.
					 */
					if (!param->values) {
						vformat_attribute_param_free (param);
						param = NULL;
						if (!colon)
							lp = g_utf8_next_char (lp);
					}
				}

				if (param
				    && !g_ascii_strcasecmp (param->name, "encoding")
				    && !g_ascii_strcasecmp (param->values->data, "quoted-printable")) {
					*quoted_printable = TRUE;
					vformat_attribute_param_free (param);
					param = NULL;
				}
			}
			else {
				if (str->len > 0) {
					char *param_name;
					if (!g_ascii_strcasecmp (str->str,
								 "quoted-printable")) {
						param_name = "ENCODING";
						*quoted_printable = TRUE;
					}
					/* apple's broken addressbook app outputs naked BASE64
					   parameters, which aren't even vcard 3.0 compliant. */
					else if (!g_ascii_strcasecmp (str->str,
								      "base64")) {
						param_name = "ENCODING";
						g_string_assign (str, "b");
					}
					else {
						param_name = "TYPE";
					}

					if (param_name) {
						param = vformat_attribute_param_new (param_name);
						vformat_attribute_param_add_value (param, str->str);
					}
					g_string_assign (str, "");
					if (!colon)
						lp = g_utf8_next_char (lp);
				}
				else {
					/* we've got an attribute with a truly empty
					   attribute parameter.  So it's of the form:
					   
					   ATTR;[PARAM=value;]*;[PARAM=value;]*:

					   (note the extra ';')

					   the only thing to do here is, well.. nothing.
					   we skip over the character if it's not a colon,
					   and the rest is handled for us: We'll either
					   continue through the loop again if we hit a ';',
					   or we'll break out correct below if it was a ':' */
					if (!colon)
						lp = g_utf8_next_char (lp);
				}
			}
			if (param && !comma) {
				vformat_attribute_add_param (attr, param);
				param = NULL;
			}
			if (colon)
				break;
		}
		else {
			g_warning ("invalid character found in parameter spec: \"%c\" String so far: %s", lp[0], str->str);
			g_string_assign (str, "");
			_skip_until (&lp, ":;");
		}
	}

	if (str)
		g_string_free (str, TRUE);

	*p = lp;
}

/* reads an entire attribute from the input buffer, leaving p pointing
   at the start of the next line (past the \r\n) */
static VFormatAttribute *_read_attribute (char **p)
{
	char *attr_group = NULL;
	char *attr_name = NULL;
	VFormatAttribute *attr = NULL;
	GString *str;
	char *lp = *p;
	
	gboolean is_qp = FALSE;

	/* first read in the group/name */
	str = g_string_new ("");
	while (*lp != '\r' && *lp != '\0') {
		if (*lp == ':' || *lp == ';') {
			if (str->len != 0) {
				/* we've got a name, break out to the value/attribute parsing */
				attr_name = g_string_free (str, FALSE);
				break;
			}
			else {
				/* a line of the form:
				 * (group.)?[:;]
				 *
				 * since we don't have an attribute
				 * name, skip to the end of the line
				 * and try again.
				 */
				g_string_free (str, TRUE);
				*p = lp;
				_skip_to_next_line(p);
				goto lose;
			}
		}
		else if (*lp == '.') {
			if (attr_group) {
				g_warning ("extra `.' in attribute specification.  ignoring extra group `%s'",
					   str->str);
				g_string_free (str, TRUE);
				str = g_string_new ("");
			}
			if (str->len != 0) {
				attr_group = g_string_free (str, FALSE);
				str = g_string_new ("");
			}
		}
		else if (g_unichar_isalnum (g_utf8_get_char (lp)) || *lp == '-' || *lp == '_' || *lp == '/') {
			str = g_string_append_unichar (str, g_utf8_get_char (lp));
		}
		else {
			g_warning ("invalid character found in attribute group/name: %c", lp[0]);
			g_string_free (str, TRUE);
			*p = lp;
			_skip_to_next_line(p);
			goto lose;
		}

		lp = g_utf8_next_char(lp);
	}

	if (!attr_name) {
		_skip_to_next_line (p);
		goto lose;
	}

	attr = vformat_attribute_new (attr_group, attr_name);
	g_free (attr_group);
	g_free (attr_name);

	if (*lp == ';') {
		/* skip past the ';' */
		lp = g_utf8_next_char(lp);
		_read_attribute_params (attr, &lp, &is_qp);
	}
	if (*lp == ':') {
		/* skip past the ':' */
		lp = g_utf8_next_char(lp);
		_read_attribute_value (attr, &lp, is_qp);
	}

	*p = lp;

	if (!attr->values)
		goto lose;

	return attr;
 lose:
	if (attr)
		vformat_attribute_free (attr);
	return NULL;
}

/* we try to be as forgiving as we possibly can here - this isn't a
 * validator.  Almost nothing is considered a fatal error.  We always
 * try to return *something*.
 */
static void _parse(VFormat *evc, const char *str)
{
	char *buf = g_strdup (str);
	char *p, *end;
	VFormatAttribute *attr;

	/* first validate the string is valid utf8 */
	if (!g_utf8_validate (buf, -1, (const char **)&end)) {
		/* if the string isn't valid, we parse as much as we can from it */
		g_warning ("invalid utf8 passed to VFormat.  Limping along.");
		*end = '\0';
	}
	
	buf = _fold_lines (buf);

	p = buf;

	attr = _read_attribute (&p);
	if (!attr || attr->group || g_ascii_strcasecmp (attr->name, "begin")) {
		g_warning ("vcard began without a BEGIN:VCARD\n");
	}
	if (attr && !g_ascii_strcasecmp (attr->name, "begin"))
		vformat_attribute_free (attr);
	else if (attr)
		vformat_add_attribute (evc, attr);

	while (*p) {
		VFormatAttribute *next_attr = _read_attribute (&p);

		if (next_attr) {
			//if (g_ascii_strcasecmp (next_attr->name, "end"))
				vformat_add_attribute (evc, next_attr);
			attr = next_attr;
		}
	}

	if (!attr || attr->group || g_ascii_strcasecmp (attr->name, "end")) {
		g_warning ("vcard ended without END:VCARD\n");
	}

	g_free (buf);
}

char *vformat_escape_string (const char *s, VFormatType type)
{
	GString *str;
	const char *p;

	str = g_string_new ("");

	/* Escape a string as described in RFC2426, section 5 */
	for (p = s; p && *p; p++) {
		switch (*p) {
		case '\n':
			str = g_string_append (str, "\\n");
			break;
		case '\r':
			if (*(p+1) == '\n')
				p++;
			str = g_string_append (str, "\\n");
			break;
		case ';':
			str = g_string_append (str, "\\;");
			break;
		case ',':
			if (type == VFORMAT_CARD_30)
				str = g_string_append (str, "\\,");
			else
				str = g_string_append_c (str, *p);
			break;
		case '\\':
			str = g_string_append (str, "\\\\");
			break;
		default:
			str = g_string_append_c (str, *p);
			break;
		}
	}

	return g_string_free (str, FALSE);
}

char*
vformat_unescape_string (const char *s)
{
	GString *str;
	const char *p;

	g_return_val_if_fail (s != NULL, NULL);

	str = g_string_new ("");

	/* Unescape a string as described in RFC2426, section 5 */
	for (p = s; *p; p++) {
		if (*p == '\\') {
			p++;
			if (*p == '\0') {
				str = g_string_append_c (str, '\\');
				break;
			}
			switch (*p) {
			case 'n':  str = g_string_append_c (str, '\n'); break;
			case 'r':  str = g_string_append_c (str, '\r'); break;
			case ';':  str = g_string_append_c (str, ';'); break;
			case ',':  str = g_string_append_c (str, ','); break;
			case '\\': str = g_string_append_c (str, '\\'); break;
			case '"': str = g_string_append_c (str, '"'); break;
			default:
				g_warning ("invalid escape, passing it through. escaped char was %i", *p);
				str = g_string_append_c (str, '\\');
				str = g_string_append_unichar (str, g_utf8_get_char(p));
				break;
			}
		}
	}

	return g_string_free (str, FALSE);
}

void
vformat_construct (VFormat *evc, const char *str)
{
	g_return_if_fail (str != NULL);

	if (*str)
		_parse (evc, str);
}

void vformat_free(VFormat *format)
{
	g_list_foreach (format->attributes, (GFunc)vformat_attribute_free, NULL);
	g_list_free (format->attributes);
}

VFormat *vformat_new_from_string (const char *str)
{
	g_return_val_if_fail (str != NULL, NULL);
	VFormat *evc = g_malloc0(sizeof(VFormat));

	vformat_construct (evc, str);

	return evc;
}

VFormat *vformat_new(void)
{
	return vformat_new_from_string ("");
}

char *vformat_to_string (VFormat *evc, VFormatType type)
{
	GList *l;
	GList *v;

	GString *str = g_string_new ("");

	switch (type) {
		case VFORMAT_CARD_21:
			str = g_string_append (str, "BEGIN:VCARD\r\nVERSION:2.1\r\n");
			break;
		case VFORMAT_CARD_30:
			str = g_string_append (str, "BEGIN:VCARD\r\nVERSION:3.0\r\n");
			break;
		case VFORMAT_TODO_10:
		case VFORMAT_EVENT_10:
			str = g_string_append (str, "BEGIN:VCALENDAR\r\nVERSION:1.0\r\n");
			break;
		case VFORMAT_TODO_20:
		case VFORMAT_EVENT_20:
			str = g_string_append (str, "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n");
			break;
		case VFORMAT_NOTE:
			str = g_string_append (str, "BEGIN:VNOTE\r\nVERSION:1.1\r\n");
			break;
	}

	for (l = evc->attributes; l; l = l->next) {
		GList *p;
		VFormatAttribute *attr = l->data;
		GString *attr_str;
		int l;

		attr_str = g_string_new ("");

		/* From rfc2425, 5.8.2
		 *
		 * contentline  = [group "."] name *(";" param) ":" value CRLF
		 */

		if (attr->group) {
			attr_str = g_string_append (attr_str, attr->group);
			attr_str = g_string_append_c (attr_str, '.');
		}
		attr_str = g_string_append (attr_str, attr->name);

		/* handle the parameters */
		for (p = attr->params; p; p = p->next) {
			VFormatParam *param = p->data;
			/* 5.8.2:
			 * param        = param-name "=" param-value *("," param-value)
			 */
			if (!g_ascii_strcasecmp (param->name, "CHARSET") && type == VFORMAT_CARD_30)
				continue;
			attr_str = g_string_append_c (attr_str, ';');
			attr_str = g_string_append (attr_str, param->name);
			if (param->values) {
				attr_str = g_string_append_c (attr_str, '=');
				for (v = param->values; v; v = v->next) {
					char *value = v->data;
					char *p = value;
					gboolean quotes = FALSE;
					while (*p) {
						if (!g_unichar_isalnum (g_utf8_get_char (p))) {
							quotes = TRUE;
							break;
						}
						p = g_utf8_next_char (p);
					}
					if (quotes)
						attr_str = g_string_append_c (attr_str, '"');
					attr_str = g_string_append (attr_str, value);
					if (quotes)
						attr_str = g_string_append_c (attr_str, '"');
					if (v->next)
						attr_str = g_string_append_c (attr_str, ',');
				}
			}
		}

		attr_str = g_string_append_c (attr_str, ':');

		for (v = attr->values; v; v = v->next) {
			char *value = v->data;
			char *escaped_value = NULL;

			escaped_value = vformat_escape_string (value, type);

			attr_str = g_string_append (attr_str, escaped_value);
			if (v->next) {
				/* XXX toshok - i hate you, rfc 2426.
				   why doesn't CATEGORIES use a ; like
				   a normal list attribute? */
				if (!strcmp (attr->name, "CATEGORIES"))
					attr_str = g_string_append_c (attr_str, ',');
				else
					attr_str = g_string_append_c (attr_str, ';');
			}

			g_free (escaped_value);
		}

		/* 5.8.2:
		 * When generating a content line, lines longer than 75
		 * characters SHOULD be folded
		 */
		l = 0;
		do {
			if (attr_str->len - l > 75) {
				l += 75;
				attr_str = g_string_insert_len (attr_str, l, CRLF " ", sizeof (CRLF " ") - 1);
			}
			else
				break;
		} while (l < attr_str->len);

		attr_str = g_string_append (attr_str, CRLF);

		str = g_string_append (str, attr_str->str);
		g_string_free (attr_str, TRUE);
	}

	switch (type) {
		case VFORMAT_CARD_21:
			str = g_string_append (str, "END:VCARD\r\n");
			break;
		case VFORMAT_CARD_30:
			str = g_string_append (str, "END:VCARD\r\n");
			break;
		case VFORMAT_TODO_10:
		case VFORMAT_EVENT_10:
			str = g_string_append (str, "END:VCALENDAR\r\n");
			break;
		case VFORMAT_TODO_20:
		case VFORMAT_EVENT_20:
			str = g_string_append (str, "END:VCALENDAR\r\n");
			break;
		case VFORMAT_NOTE:
			str = g_string_append (str, "END:VNOTE\r\n");
			break;
	}
	
	return g_string_free (str, FALSE);
}

void vformat_dump_structure (VFormat *evc)
{
	GList *a;
	GList *v;
	int i;

	printf ("VFormat\n");
	for (a = evc->attributes; a; a = a->next) {
		GList *p;
		VFormatAttribute *attr = a->data;
		printf ("+-- %s\n", attr->name);
		if (attr->params) {
			printf ("    +- params=\n");

			for (p = attr->params, i = 0; p; p = p->next, i++) {
				VFormatParam *param = p->data;
				printf ("    |   [%d] = %s", i,param->name);
				printf ("(");
				for (v = param->values; v; v = v->next) {
					char *value = vformat_escape_string ((char*)v->data, VFORMAT_CARD_21);
					printf ("%s", value);
					if (v->next)
						printf (",");
					g_free (value);
				}

				printf (")\n");
			}
		}
		printf ("    +- values=\n");
		for (v = attr->values, i = 0; v; v = v->next, i++) {
			printf ("        [%d] = `%s'\n", i, (char*)v->data);
		}
	}
}

VFormatAttribute *vformat_attribute_new (const char *attr_group, const char *attr_name)
{
	VFormatAttribute *attr;

	attr = g_new0 (VFormatAttribute, 1);

	attr->group = g_strdup (attr_group);
	attr->name = g_strdup (attr_name);

	return attr;
}

void
vformat_attribute_free (VFormatAttribute *attr)
{
	g_return_if_fail (attr != NULL);

	g_free (attr->group);
	g_free (attr->name);

	vformat_attribute_remove_values (attr);

	vformat_attribute_remove_params (attr);

	g_free (attr);
}

VFormatAttribute*
vformat_attribute_copy (VFormatAttribute *attr)
{
	VFormatAttribute *a;
	GList *p;

	g_return_val_if_fail (attr != NULL, NULL);

	a = vformat_attribute_new (vformat_attribute_get_group (attr),
				   vformat_attribute_get_name (attr));

	for (p = attr->values; p; p = p->next)
		vformat_attribute_add_value (a, p->data);

	for (p = attr->params; p; p = p->next)
		vformat_attribute_add_param (a, vformat_attribute_param_copy (p->data));

	return a;
}

void
vformat_remove_attributes (VFormat *evc, const char *attr_group, const char *attr_name)
{
	GList *attr;

	g_return_if_fail (attr_name != NULL);

	attr = evc->attributes;
	while (attr) {
		GList *next_attr;
		VFormatAttribute *a = attr->data;

		next_attr = attr->next;

		if (((!attr_group && !a->group) ||
		     (attr_group && !g_ascii_strcasecmp (attr_group, a->group))) &&
		    ((!attr_name && !a->name) || !g_ascii_strcasecmp (attr_name, a->name))) {

			/* matches, remove/delete the attribute */
			evc->attributes = g_list_remove_link (evc->attributes, attr);

			vformat_attribute_free (a);
		}

		attr = next_attr;
	}
}

void
vformat_remove_attribute (VFormat *evc, VFormatAttribute *attr)
{
	g_return_if_fail (attr != NULL);

	evc->attributes = g_list_remove (evc->attributes, attr);
	vformat_attribute_free (attr);
}

void
vformat_add_attribute (VFormat *evc, VFormatAttribute *attr)
{
	g_return_if_fail (attr != NULL);

	evc->attributes = g_list_append (evc->attributes, attr);
}

void
vformat_add_attribute_with_value (VFormat *VFormat,
				  VFormatAttribute *attr, const char *value)
{
	g_return_if_fail (attr != NULL);

	vformat_attribute_add_value (attr, value);

	vformat_add_attribute (VFormat, attr);
}

void
vformat_add_attribute_with_values (VFormat *VFormat, VFormatAttribute *attr, ...)
{
	va_list ap;
	char *v;

	g_return_if_fail (attr != NULL);

	va_start (ap, attr);

	while ((v = va_arg (ap, char*))) {
		vformat_attribute_add_value (attr, v);
	}

	va_end (ap);

	vformat_add_attribute (VFormat, attr);
}

void
vformat_attribute_add_value (VFormatAttribute *attr, const char *value)
{
	g_return_if_fail (attr != NULL);

	attr->values = g_list_append (attr->values, g_strdup (value));
}

void
vformat_attribute_add_value_decoded (VFormatAttribute *attr, const char *value, int len)
{
	g_return_if_fail (attr != NULL);

	switch (attr->encoding) {
		case VF_ENCODING_RAW:
			g_warning ("can't add_value_decoded with an attribute using RAW encoding.  you must set the ENCODING parameter first");
			break;
		case VF_ENCODING_BASE64: {
			char *b64_data = base64_encode_simple (value, len);
			GString *decoded = g_string_new_len (value, len);
	
			/* make sure the decoded list is up to date */
			vformat_attribute_get_values_decoded (attr);
	
			attr->values = g_list_append (attr->values, b64_data);
			attr->decoded_values = g_list_append (attr->decoded_values, decoded);
			break;
		}
		case VF_ENCODING_QP: {
			char *qp_data = quoted_encode_simple ((unsigned char*)value, len);
			GString *decoded = g_string_new (value);
	
			/* make sure the decoded list is up to date */
			vformat_attribute_get_values_decoded (attr);
	
			attr->values = g_list_append (attr->values, qp_data);
			attr->decoded_values = g_list_append (attr->decoded_values, decoded);
			break;
		}
		case VF_ENCODING_8BIT: {
			char *data = g_strdup(value);
			GString *decoded = g_string_new (value);
	
			/* make sure the decoded list is up to date */
			vformat_attribute_get_values_decoded (attr);
	
			attr->values = g_list_append (attr->values, data);
			attr->decoded_values = g_list_append (attr->decoded_values, decoded);
			break;
		}
	}
}

void
vformat_attribute_add_values (VFormatAttribute *attr, ...)
{
	va_list ap;
	char *v;

	g_return_if_fail (attr != NULL);

	va_start (ap, attr);

	while ((v = va_arg (ap, char*))) {
		vformat_attribute_add_value (attr, v);
	}

	va_end (ap);
}

static void
free_gstring (GString *str)
{
	g_string_free (str, TRUE);
}

void
vformat_attribute_remove_values (VFormatAttribute *attr)
{
	g_return_if_fail (attr != NULL);

	g_list_foreach (attr->values, (GFunc)g_free, NULL);
	g_list_free (attr->values);
	attr->values = NULL;

	g_list_foreach (attr->decoded_values, (GFunc)free_gstring, NULL);
	g_list_free (attr->decoded_values);
	attr->decoded_values = NULL;
}

void
vformat_attribute_remove_params (VFormatAttribute *attr)
{
	g_return_if_fail (attr != NULL);

	g_list_foreach (attr->params, (GFunc)vformat_attribute_param_free, NULL);
	g_list_free (attr->params);
	attr->params = NULL;

	/* also remove the cached encoding on this attribute */
	attr->encoding_set = FALSE;
	attr->encoding = VF_ENCODING_RAW;
}

VFormatParam*
vformat_attribute_param_new (const char *name)
{
	VFormatParam *param = g_new0 (VFormatParam, 1);
	param->name = g_strdup (name);

	return param;
}

void
vformat_attribute_param_free (VFormatParam *param)
{
	g_return_if_fail (param != NULL);

	g_free (param->name);

	vformat_attribute_param_remove_values (param);

	g_free (param);
}

VFormatParam*
vformat_attribute_param_copy (VFormatParam *param)
{
	VFormatParam *p;
	GList *l;

	g_return_val_if_fail (param != NULL, NULL);

	p = vformat_attribute_param_new (vformat_attribute_param_get_name (param));

	for (l = param->values; l; l = l->next) {
		vformat_attribute_param_add_value (p, l->data);
	}

	return p;
}

void
vformat_attribute_add_param (VFormatAttribute *attr,
			     VFormatParam *param)
{
	g_return_if_fail (attr != NULL);
	g_return_if_fail (param != NULL);

	attr->params = g_list_append (attr->params, param);

	/* we handle our special encoding stuff here */

	if (!g_ascii_strcasecmp (param->name, "ENCODING")) {
		if (attr->encoding_set) {
			g_warning ("ENCODING specified twice");
			return;
		}

		if (param->values && param->values->data) {
			if (!g_ascii_strcasecmp ((char*)param->values->data, "b"))
				attr->encoding = VF_ENCODING_BASE64;
			else if (!g_ascii_strcasecmp ((char*)param->values->data, "QUOTED-PRINTABLE"))
				attr->encoding = VF_ENCODING_QP;
			else if (!g_ascii_strcasecmp ((char *)param->values->data, "8BIT"))
				attr->encoding = VF_ENCODING_8BIT;
			else {
				g_warning ("Unknown value `%s' for ENCODING parameter.  values will be treated as raw",
					   (char*)param->values->data);
			}

			attr->encoding_set = TRUE;
		}
		else {
			g_warning ("ENCODING parameter added with no value");
		}
	}
}

void
vformat_attribute_param_add_value (VFormatParam *param,
				   const char *value)
{
	g_return_if_fail (param != NULL);

	param->values = g_list_append (param->values, g_strdup (value));
}

void
vformat_attribute_param_add_values (VFormatParam *param,
				    ...)
{
	va_list ap;
	char *v;

	g_return_if_fail (param != NULL);

	va_start (ap, param);

	while ((v = va_arg (ap, char*))) {
		vformat_attribute_param_add_value (param, v);
	}

	va_end (ap);
}

void
vformat_attribute_add_param_with_value (VFormatAttribute *attr, const char *name, const char *value)
{
	g_return_if_fail (attr != NULL);
	g_return_if_fail (name != NULL);
	
	if (!value)
		return;
	
	VFormatParam *param = vformat_attribute_param_new(name);

	vformat_attribute_param_add_value (param, value);

	vformat_attribute_add_param (attr, param);
}

void
vformat_attribute_add_param_with_values (VFormatAttribute *attr,
					 VFormatParam *param, ...)
{
	va_list ap;
	char *v;

	g_return_if_fail (attr != NULL);
	g_return_if_fail (param != NULL);

	va_start (ap, param);

	while ((v = va_arg (ap, char*))) {
		vformat_attribute_param_add_value (param, v);
	}

	va_end (ap);

	vformat_attribute_add_param (attr, param);
}

void
vformat_attribute_param_remove_values (VFormatParam *param)
{
	g_return_if_fail (param != NULL);

	g_list_foreach (param->values, (GFunc)g_free, NULL);
	g_list_free (param->values);
	param->values = NULL;
}

GList*
vformat_get_attributes (VFormat *format)
{
	return format->attributes;
}

const char*
vformat_attribute_get_group (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, NULL);

	return attr->group;
}

const char*
vformat_attribute_get_name (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, NULL);

	return attr->name;
}

GList*
vformat_attribute_get_values (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, NULL);

	return attr->values;
}

GList*
vformat_attribute_get_values_decoded (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, NULL);

	if (!attr->decoded_values) {
		GList *l;
		switch (attr->encoding) {
		case VF_ENCODING_RAW:
		case VF_ENCODING_8BIT:
			for (l = attr->values; l; l = l->next)
				attr->decoded_values = g_list_append (attr->decoded_values, g_string_new ((char*)l->data));
			break;
		case VF_ENCODING_BASE64:
			for (l = attr->values; l; l = l->next) {
				char *decoded = g_strdup ((char*)l->data);
				int len = base64_decode_simple (decoded, strlen (decoded));
				attr->decoded_values = g_list_append (attr->decoded_values, g_string_new_len (decoded, len));
				g_free (decoded);
			}
			break;
		case VF_ENCODING_QP:
			for (l = attr->values; l; l = l->next) {
				if (!(l->data))
					continue;
				char *decoded = g_strdup ((char*)l->data);
				int len = quoted_decode_simple (decoded, strlen (decoded));
				attr->decoded_values = g_list_append (attr->decoded_values, g_string_new_len (decoded, len));
				g_free (decoded);
			}
			break;
		}
	}

	return attr->decoded_values;
}

gboolean
vformat_attribute_is_single_valued (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, FALSE);

	if (attr->values == NULL
	    || attr->values->next != NULL)
		return FALSE;

	return TRUE;
}

char*
vformat_attribute_get_value (VFormatAttribute *attr)
{
	GList *values;

	g_return_val_if_fail (attr != NULL, NULL);

	values = vformat_attribute_get_values (attr);

	if (!vformat_attribute_is_single_valued (attr))
		g_warning ("vformat_attribute_get_value called on multivalued attribute");

	return values ? g_strdup ((char*)values->data) : NULL;
}

GString*
vformat_attribute_get_value_decoded (VFormatAttribute *attr)
{
	GList *values;
	GString *str = NULL;

	g_return_val_if_fail (attr != NULL, NULL);

	values = vformat_attribute_get_values_decoded (attr);

	if (!vformat_attribute_is_single_valued (attr))
		g_warning ("vformat_attribute_get_value_decoded called on multivalued attribute");

	if (values)
		str = values->data;

	return str ? g_string_new_len (str->str, str->len) : NULL;
}

const char *vformat_attribute_get_nth_value(VFormatAttribute *attr, int nth)
{
	GList *values = vformat_attribute_get_values_decoded(attr);
	if (!values)
		return NULL;
	GString *retstr = (GString *)g_list_nth_data(values, nth);
	if (!retstr)
		return NULL;
	
	if (!g_utf8_validate(retstr->str, -1, NULL)) {
		values = vformat_attribute_get_values(attr);
		if (!values)
			return NULL;
		return g_list_nth_data(values, nth);
	}
	
	return retstr->str;
}

gboolean
vformat_attribute_has_type (VFormatAttribute *attr, const char *typestr)
{
	GList *params;
	GList *p;

	g_return_val_if_fail (attr != NULL, FALSE);
	g_return_val_if_fail (typestr != NULL, FALSE);

	params = vformat_attribute_get_params (attr);

	for (p = params; p; p = p->next) {
		VFormatParam *param = p->data;

		if (!strcasecmp (vformat_attribute_param_get_name (param), "TYPE")) {
			GList *values = vformat_attribute_param_get_values (param);
			GList *v;

			for (v = values; v; v = v->next) {
				if (!strcasecmp ((char*)v->data, typestr))
					return TRUE;
			}
		}
	}

	return FALSE;
}


gboolean vformat_attribute_has_param(VFormatAttribute *attr, const char *name)
{
	g_return_val_if_fail (attr != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	
	GList *params = vformat_attribute_get_params(attr);
	GList *p;
	for (p = params; p; p = p->next) {
		VFormatParam *param = p->data;
		if (!strcasecmp(name, vformat_attribute_param_get_name(param)))
			return TRUE;
	}
	return FALSE;
}

GList*
vformat_attribute_get_params (VFormatAttribute *attr)
{
	g_return_val_if_fail (attr != NULL, NULL);

	return attr->params;
}

const char*
vformat_attribute_param_get_name (VFormatParam *param)
{
	g_return_val_if_fail (param != NULL, NULL);

	return param->name;
}

GList*
vformat_attribute_param_get_values (VFormatParam *param)
{
	g_return_val_if_fail (param != NULL, NULL);

	return param->values;
}

const char *vformat_attribute_param_get_nth_value(VFormatParam *param, int nth)
{
	const char *ret = NULL;
	GList *values = vformat_attribute_param_get_values(param);
	if (!values)
		return NULL;
	ret = g_list_nth_data(values, nth);
	return ret;
}

static const char *base64_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//static unsigned char _evc_base64_rank[256];

static void base64_init(char *rank)
{
	int i;

	memset(rank, 0xff, sizeof(rank));
	for (i=0;i<64;i++) {
		rank[(unsigned int)base64_alphabet[i]] = i;
	}
	rank['='] = 0;
}

/* call this when finished encoding everything, to
   flush off the last little bit */
static size_t base64_encode_close(unsigned char *in, size_t inlen, gboolean break_lines, unsigned char *out, int *state, int *save)
{
	int c1, c2;
	unsigned char *outptr = out;

	if (inlen>0)
		outptr += base64_encode_step(in, inlen, break_lines, outptr, state, save);

	c1 = ((unsigned char *)save)[1];
	c2 = ((unsigned char *)save)[2];

	switch (((char *)save)[0]) {
	case 2:
		outptr[2] = base64_alphabet[ ( (c2 &0x0f) << 2 ) ];
		g_assert(outptr[2] != 0);
		goto skip;
	case 1:
		outptr[2] = '=';
	skip:
		outptr[0] = base64_alphabet[ c1 >> 2 ];
		outptr[1] = base64_alphabet[ c2 >> 4 | ( (c1&0x3) << 4 )];
		outptr[3] = '=';
		outptr += 4;
		break;
	}
	if (break_lines)
		*outptr++ = '\n';

	*save = 0;
	*state = 0;

	return outptr-out;
}

/*
  performs an 'encode step', only encodes blocks of 3 characters to the
  output at a time, saves left-over state in state and save (initialise to
  0 on first invocation).
*/
static size_t base64_encode_step(unsigned char *in, size_t len, gboolean break_lines, unsigned char *out, int *state, int *save)
{
	register unsigned char *inptr, *outptr;

	if (len<=0)
		return 0;

	inptr = in;
	outptr = out;

	if (len + ((char *)save)[0] > 2) {
		unsigned char *inend = in+len-2;
		register int c1, c2, c3;
		register int already;

		already = *state;

		switch (((char *)save)[0]) {
		case 1:	c1 = ((unsigned char *)save)[1]; goto skip1;
		case 2:	c1 = ((unsigned char *)save)[1];
			c2 = ((unsigned char *)save)[2]; goto skip2;
		}
		
		/* yes, we jump into the loop, no i'm not going to change it, it's beautiful! */
		while (inptr < inend) {
			c1 = *inptr++;
		skip1:
			c2 = *inptr++;
		skip2:
			c3 = *inptr++;
			*outptr++ = base64_alphabet[ c1 >> 2 ];
			*outptr++ = base64_alphabet[ c2 >> 4 | ( (c1&0x3) << 4 ) ];
			*outptr++ = base64_alphabet[ ( (c2 &0x0f) << 2 ) | (c3 >> 6) ];
			*outptr++ = base64_alphabet[ c3 & 0x3f ];
			/* this is a bit ugly ... */
			if (break_lines && (++already)>=19) {
				*outptr++='\n';
				already = 0;
			}
		}

		((char *)save)[0] = 0;
		len = 2-(inptr-inend);
		*state = already;
	}

	if (len>0) {
		register char *saveout;

		/* points to the slot for the next char to save */
		saveout = & (((char *)save)[1]) + ((char *)save)[0];

		/* len can only be 0 1 or 2 */
		switch(len) {
		case 2:	*saveout++ = *inptr++;
		case 1:	*saveout++ = *inptr++;
		}
		((char *)save)[0]+=len;
	}

	return outptr-out;
}


/**
 * base64_decode_step: decode a chunk of base64 encoded data
 * @in: input stream
 * @len: max length of data to decode
 * @out: output stream
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been decoded
 *
 * Decodes a chunk of base64 encoded data
 **/
static size_t base64_decode_step(unsigned char *in, size_t len, unsigned char *out, int *state, unsigned int *save)
{
	unsigned char base64_rank[256];
	base64_init((char*)base64_rank);
	
	register unsigned char *inptr, *outptr;
	unsigned char *inend, c;
	register unsigned int v;
	int i;

	inend = in+len;
	outptr = out;

	/* convert 4 base64 bytes to 3 normal bytes */
	v=*save;
	i=*state;
	inptr = in;
	while (inptr<inend) {
		c = base64_rank[*inptr++];
		if (c != 0xff) {
			v = (v<<6) | c;
			i++;
			if (i==4) {
				*outptr++ = v>>16;
				*outptr++ = v>>8;
				*outptr++ = v;
				i=0;
			}
		}
	}

	*save = v;
	*state = i;

	/* quick scan back for '=' on the end somewhere */
	/* fortunately we can drop 1 output char for each trailing = (upto 2) */
	i=2;
	while (inptr>in && i) {
		inptr--;
		if (base64_rank[*inptr] != 0xff) {
			if (*inptr == '=' && outptr>out)
				outptr--;
			i--;
		}
	}

	/* if i!= 0 then there is a truncation error! */
	return outptr-out;
}

char *base64_encode_simple (const char *data, size_t len)
{
	unsigned char *out;
	int state = 0, outlen;
	unsigned int save = 0;

	g_return_val_if_fail (data != NULL, NULL);

	out = g_malloc (len * 4 / 3 + 5);
	outlen = base64_encode_close ((unsigned char *)data, len, FALSE,
				      out, &state, (int*)&save);
	out[outlen] = '\0';
	return (char *)out;
}

size_t base64_decode_simple (char *data, size_t len)
{
	int state = 0;
	unsigned int save = 0;

	g_return_val_if_fail (data != NULL, 0);

	return base64_decode_step ((unsigned char *)data, len,
					(unsigned char *)data, &state, &save);
}

char *quoted_encode_simple(const unsigned char *string, int len)
{
	GString *tmp = g_string_new("");
	
	int i = 0;
	while(string[i] != 0) {
		if (string[i] > 127 || string[i] == 13 || string[i] == 10 || string[i] == '=') {
			g_string_append_printf(tmp, "=%02X", string[i]);
		} else {
			g_string_append_c(tmp, string[i]);
		}
		i++;
	}
	
	char *ret = tmp->str;
	g_string_free(tmp, FALSE);
	return ret;
}


size_t quoted_decode_simple (char *data, size_t len)
{
	g_return_val_if_fail (data != NULL, 0);

	GString *string = g_string_new(data);
	if (!string)
		return 0;

	char hex[5];
	hex[4] = 0;

	while (1) {
		//Get the index of the next encoded char
		int i = strcspn(string->str, "=");
		if (i >= strlen(string->str))
			break;
		
		strcpy(hex, "0x");
		strncat(hex, &string->str[i + 1], 2);
		char rep = ((int)(strtod(hex, NULL)));
		g_string_erase(string, i, 2);
		g_string_insert_c(string, i, rep);
	}
	
	memset(data, 0, strlen(data));
	strcpy(data, string->str);
	g_string_free(string, 1);
	
	return strlen(data);
}
