/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Michael Zucchi <notzed@helixcode.com>
 *           Jeffrey Stedfast <fejj@helixcode.com>
 *
 *  Copyright 2000 Helix Code, Inc. (www.helixcode.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gmime-utils.h"
#include "gmime-table-private.h"
#include "gmime-part.h"
#include "gmime-charset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define d(x)

#define GMIME_UUENCODE_CHAR(c) ((c) ? (c) + ' ' : '`')
#define	GMIME_UUDECODE_CHAR(c) (((c) - ' ') & 077)

#ifndef HAVE_ISBLANK
#define isblank(c) (c == ' ' || c == '\t')
#endif

static char *base64_alphabet =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char tohex[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static unsigned char gmime_base64_rank[256] = {
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
	255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
	255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

static unsigned char gmime_uu_rank[256] = {
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
};


/* hrm, is there a library for this shit? */
static struct {
	char *name;
	int offset;
} tz_offsets [] = {
	{ "UT", 0 },
	{ "GMT", 0 },
	{ "EST", -500 },	/* these are all US timezones.  bloody yanks */
	{ "EDT", -400 },
	{ "CST", -600 },
	{ "CDT", -500 },
	{ "MST", -700 },
	{ "MDT", -600 },
	{ "PST", -800 },
	{ "PDT", -700 },
	{ "Z", 0 },
	{ "A", -100 },
	{ "M", -1200 },
	{ "N", 100 },
	{ "Y", 1200 },
};

static char *tm_months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static char *tm_days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};


/**
 * g_mime_utils_header_format_date:
 * @time: time_t date representation
 * @offset: Timezone offset
 *
 * Allocates a string buffer containing the rfc822 formatted date
 * string represented by @time and @offset.
 *
 * Returns a valid string representation of the date.
 **/
char *
g_mime_utils_header_format_date (time_t time, int offset)
{
	struct tm tm;
	
	time += ((offset / 100) * (60 * 60)) + (offset % 100) * 60;
	
	memcpy (&tm, gmtime (&time), sizeof (tm));
	
	return g_strdup_printf ("%s, %02d %s %04d %02d:%02d:%02d %+05d",
				tm_days[tm.tm_wday], tm.tm_mday,
				tm_months[tm.tm_mon],
				tm.tm_year + 1900,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				offset);
}

/* This is where it gets ugly... */

static GList *
datetok (const char *date)
{
	GList *tokens = NULL;
	char *token, *start, *end;
	
	g_return_val_if_fail (date != NULL, NULL);
	
	start = (char *) date;
	while (*start) {
		/* kill leading whitespace */
		for ( ; *start && isspace ((int)*start); start++);
		
		/* find the end of this token */
		for (end = start; *end && !isspace ((int)*end); end++);
		
		token = g_strndup (start, (end - start));
		
		if (token && *token)
			tokens = g_list_append (tokens, token);
		else
			g_free (token);
		
		if (*end)
			start = end + 1;
		else
			break;
	}
	
	return tokens;
}

#if 0
static gint
get_days_in_month (gint month, gint year)
{
        switch (month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
	        return 31;
	case 4:
	case 6:
	case 9:
	case 11:
	        return 30;
	case 2:
	        if (g_date_is_leap_year (year))
		        return 29;
		else
		        return 28;
	default:
	        return 0;
	}
}
#endif

static int
get_wday (char *str)
{
	gint i;
	
	g_return_val_if_fail (str != NULL, -1);
	
	for (i = 0; i < 7; i++)
		if (!strncasecmp (str, tm_days[i], 3))
			return i;
	
	return -1;  /* unknown week day */
}

static int
get_mday (char *str)
{
	char *p;
	int mday;
	
	g_return_val_if_fail (str != NULL, -1);
	
	for (p = str; *p; p++)
		if (!isdigit ((int) *p))
			return -1;
	
	mday = atoi (str);
	
	if (mday < 0 || mday > 31)
		mday = -1;
	
	return mday;
}

static int
get_month (char *str)
{
	int i;
	
	g_return_val_if_fail (str != NULL, -1);
	
	for (i = 0; i < 12; i++)
		if (!strncasecmp (str, tm_months[i], 3))
			return i;
	
	return -1;  /* unknown month */
}

static int
get_year (const char *str)
{
	const char *p;
	int year;
	
	for (p = str; *p; p++)
		if (!isdigit ((int) *p))
			return -1;
	
	year = atoi (str);
	
	if (year < 100)
		year += (year < 70) ? 2000 : 1900;
	
	if (year < 1969)
		return -1;
	
	return year;
}

static gboolean
get_time (const char *in, int *hour, int *min, int *sec)
{
	const char *p;
	int colons = 0;
	gboolean digits = TRUE;
	
	for (p = in; *p && digits; p++) {
		if (*p == ':')
			colons++;
		else if (!isdigit ((int) *p))
			digits = FALSE;
	}
	
	/* Ameol software doesn't put the seconds on the time */
	if (!digits || (colons != 2 && colons != 1))
		return FALSE;
	
	if (colons == 2)
		return sscanf (in, "%d:%d:%d", hour, min, sec) == 3;
	else {
		*sec = 0;
		return sscanf (in, "%d:%d", hour, min) == 2;
	}
}

static int
get_tzone (GList **token)
{
	int tz = -1;
	int i;
	
	for (i = 0; *token && i < 2; *token = (*token)->next, i++) {
		char *str = (*token)->data;
		
		if (*str == '+' || *str == '-') {
			tz = atoi (str);
			return tz;
		} else {
			int t;
			
			if (*str == '(')
				str++;
			
			for (t = 0; t < 15; t++)
				if (!strncmp (str, tz_offsets[t].name, strlen (tz_offsets[t].name))) {
					tz = tz_offsets[t].offset;
					return tz;
				}
		}
	}
	
	return -1;
}

static time_t
parse_rfc822_date (GList *tokens, int *tzone)
{
	GList *token;
	struct tm tm;
	time_t t;
	int hour, min, sec, offset, n;
	
	g_return_val_if_fail (tokens != NULL, (time_t) 0);
	
	token = tokens;
	
	memset ((void *) &tm, 0, sizeof (struct tm));
	
	if ((n = get_wday (token->data)) != -1) {
		/* not all dates may have this... */
		tm.tm_wday = n;
		token = token->next;
	}
	
	/* get the mday */
	if (!token || (n = get_mday (token->data)) == -1)
		return (time_t) 0;
	
	tm.tm_mday = n;
	token = token->next;
	
	/* get the month */
	if (!token || (n = get_month (token->data)) == -1)
		return (time_t) 0;
	
	tm.tm_mon = n;
	token = token->next;
	
	/* get the year */
	if (!token || (n = get_year (token->data)) == -1)
		return (time_t) 0;
	
	tm.tm_year = n - 1900;
	token = token->next;
	
	/* get the hour/min/sec */
	if (!token || !get_time (token->data, &hour, &min, &sec))
		return (time_t) 0;
	
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
	token = token->next;
	
	/* get the timezone */
	if (!token || (n = get_tzone (&token)) == -1) {
		/* I guess we assume tz is GMT? */			
		offset = 0;
	} else {
		offset = n;
	}
	
	t = mktime (&tm);
#if defined(HAVE_TIMEZONE)
	t -= timezone;
#elif defined(HAVE_TM_GMTOFF)
	t += tm.tm_gmtoff;
#else
#error Neither HAVE_TIMEZONE nor HAVE_TM_GMTOFF defined. Rerun autoheader, autoconf, etc.
#endif
	
	/* t is now GMT of the time we want, but not offset by the timezone ... */
	
	/* this should convert the time to the GMT equiv time */
	t -= ((offset / 100) * 60 * 60) + (offset % 100) * 60;
	
	if (tzone)
		*tzone = offset;
	
	return t;
}

static time_t
parse_broken_date (GList *tokens, int *tzone)
{
#if 0
	GList *token;
	struct tm tm;
	int hour, min, sec, n;
	
	if (tzone)
		*tzone = 0;
#endif	
	return (time_t) 0;
}


/**
 * g_mime_utils_header_decode_date:
 * @in: input date string
 * @saveoffset:
 *
 * Decodes the rfc822 date string and saves the GMT offset into
 * @saveoffset if non-NULL.
 *
 * Returns the time_t representation of the date string specified by
 * @in. If 'saveoffset' is non-NULL, the value of the timezone offset
 * will be stored.
 **/
time_t
g_mime_utils_header_decode_date (const char *in, int *saveoffset)
{
	GList *token, *tokens;
	time_t date;
	
	tokens = datetok (in);
	
	date = parse_rfc822_date (tokens, saveoffset);
	if (!date)
		date = parse_broken_date (tokens, saveoffset);
	
	/* cleanup */
	token = tokens;
	while (token) {
		g_free (token->data);
		token = token->next;
	}
	g_list_free (tokens);
	
	return date;
}


/**
 * g_mime_utils_header_fold:
 * @in: input header string
 *
 * Folds a header according to the rules in rfc822.
 *
 * Returns an allocated string containing the folded header.
 **/
char *
g_mime_utils_header_fold (const char *in)
{
	const char *inptr, *space;
	size_t len, outlen, i;
	GString *out;
	char *ret;
	
	inptr = in;
	len = strlen (in);
	if (len <= GMIME_FOLD_LEN)
		return g_strdup (in);
	
	out = g_string_new ("");
	outlen = 0;
	while (*inptr) {
		len = strcspn (inptr, " \t");
		
		if (outlen + len > GMIME_FOLD_LEN) {
			g_string_append (out, "\n\t");
			outlen = 1;
			
			/* check for very long words, just cut them up */
			while (outlen + len > GMIME_FOLD_LEN) {
				for (i = 0; i < GMIME_FOLD_LEN - outlen; i++)
					g_string_append_c (out, inptr[i]);
				inptr += GMIME_FOLD_LEN - outlen;
				len -= GMIME_FOLD_LEN - outlen;
				g_string_append (out, "\n\t");
				outlen = 1;
			}
		} else if (len > 0) {
			outlen += len;
			for (i = 0; i < len; i++) {
				g_string_append_c (out, inptr[i]);
			}
			inptr += len;
		} else {
			g_string_append_c (out, *inptr++);
		}
	}
	
	ret = out->str;
	g_string_free (out, FALSE);
	
	return ret;	
}


/**
 * g_mime_utils_header_printf:
 * @format: string format
 * @Varargs: arguments
 *
 * Allocates a buffer containing a formatted header specified by the
 * @Varargs.
 *
 * Returns an allocated string containing the folded header specified
 * by @format and the following arguments.
 **/
char *
g_mime_utils_header_printf (const char *format, ...)
{
	char *buf, *ret;
	va_list ap;
	
	va_start (ap, format);
	buf = g_strdup_vprintf (format, ap);
	va_end (ap);
	
	ret = g_mime_utils_header_fold (buf);
	g_free (buf);
	
	return ret;
}

static gboolean
need_quotes (const char *string)
{
	gboolean quoted = FALSE;
	const char *inptr;
	
	inptr = string;
	
	while (*inptr) {
		if (*inptr == '\\')
			inptr++;
		else if (*inptr == '"')
			quoted = !quoted;
		else if (!quoted && (is_tspecial (*inptr) || *inptr == '.'))
			return TRUE;
		
		if (*inptr)
			inptr++;
	}
	
	return FALSE;
}

/**
 * g_mime_utils_quote_string:
 * @string: input string
 *
 * Quotes @string as needed according to the rules in rfc2045.
 * 
 * Returns an allocated string containing the escaped and quoted (if
 * needed to be) input string. The decision to quote the string is
 * based on whether or not the input string contains any 'tspecials'
 * as defined by rfc2045.
 **/
char *
g_mime_utils_quote_string (const char *string)
{
	gboolean quote;
	const char *c;
	char *qstring;
	GString *out;
	
	out = g_string_new ("");
	quote = need_quotes (string);
	
	for (c = string; *c; c++) {
		if ((*c == '"' && quote) || *c == '\\')
			g_string_append_c (out, '\\');
		
		g_string_append_c (out, *c);
	}
	
	if (quote) {
		g_string_prepend_c (out, '"');
		g_string_append_c (out, '"');
	}
	
	qstring = out->str;
	g_string_free (out, FALSE);
	
	return qstring;
}


/**
 * g_mime_utils_unquote_string: Unquote a string.
 * @string: string
 * 
 * Unquotes and unescapes a string.
 **/
void
g_mime_utils_unquote_string (char *string)
{
	/* if the string is quoted, unquote it */
	char *inptr, *inend;
	
	if (!string)
		return;
	
	inptr = string;
	inend = string + strlen (string);
	
	/* get rid of the wrapping quotes */
	if (*inptr == '"' && *(inend - 1) == '"') {
		inend--;
		*inend = '\0';
		if (*inptr)
			memmove (inptr, inptr + 1, inend - inptr);
	}
	
	/* un-escape the string */
	inend--;
	while (inptr < inend) {
		if (*inptr == '\\') {
			memmove (inptr, inptr + 1, inend - inptr);
			inend--;
		}
		
		inptr++;
	}
}


/**
 * g_mime_utils_text_is_8bit:
 * @text: text to check for 8bit chars
 * @len: text length
 *
 * Determines if @text contains 8bit characters within the first @len
 * bytes.
 *
 * Returns TRUE if the text contains 8bit characters or FALSE
 * otherwise.
 **/
gboolean
g_mime_utils_text_is_8bit (const guchar *text, guint len)
{
	guchar *c, *inend;
	
	g_return_val_if_fail (text != NULL, FALSE);
	
	inend = (guchar *) text + len;
	for (c = (guchar *) text; c < inend; c++)
		if (*c > (guchar) 127)
			return TRUE;
	
	return FALSE;
}


/**
 * g_mime_utils_best_encoding:
 * @text: text to encode
 * @len: text length
 *
 * Determines the best content encoding for the first @len bytes of
 * @text.
 *
 * Returns a GMimePartEncodingType that is determined to be the best
 * encoding type for the specified block of text. ("best" in this
 * particular case means best compression)
 **/
GMimePartEncodingType
g_mime_utils_best_encoding (const guchar *text, guint len)
{
	guchar *ch, *inend;
	gint count = 0;
	
	inend = (guchar *) text + len;
	for (ch = (guchar *) text; ch < inend; ch++)
		if (*ch > (guchar) 127)
			count++;
	
	if ((float) count <= len * 0.17)
		return GMIME_PART_ENCODING_QUOTEDPRINTABLE;
	else
		return GMIME_PART_ENCODING_BASE64;
}

/* this decodes rfc2047's version of quoted-printable */
static gint
quoted_decode (const guchar *in, gint len, guchar *out)
{
	register const guchar *inptr;
	register guchar *outptr;
	const guchar *inend;
	guchar c, c1;
	gboolean err = FALSE;
	
	inend = in + len;
	outptr = out;
	
	inptr = in;
	while (inptr < inend) {
		c = *inptr++;
		if (c == '=') {
			if (inend - inptr >= 2) {
				c = toupper (*inptr++);
				c1 = toupper (*inptr++);
				*outptr++ = (((c >= 'A' ? c - 'A' + 10 : c - '0') & 0x0f) << 4)
					| ((c1 >= 'A' ? c1 - 'A' + 10 : c1 - '0') & 0x0f);
			} else {
				/* data was truncated */
				err = TRUE;
				break;
			}
		} else if (c == '_') {
			/* _'s are an rfc2047 shortcut for encoding spaces */
			*outptr++ = ' ';
		} else if (isblank (c) || strchr (CHARS_ESPECIAL, c)) {
			/* FIXME: this is an error! ignore for now ... */
			err = TRUE;
			break;
		} else {
			*outptr++ = c;
		}
	}
	
	if (!err) {
		return (outptr - out);
	}
	
	return -1;
}

#define is_8bit_word_encoded(atom, len) (len >= 7 && !strncmp (atom, "=?", 2) && !strncmp (atom + len - 2, "?=", 2))

static guchar *
decode_encoded_8bit_word (const guchar *word)
{
	guchar *inptr, *inend;
	guint len;
	
	len = strlen (word);
	
	inptr = (guchar *) word + 2;
	inend = (guchar *) word + len - 2;
	
	inptr = memchr (inptr, '?', inend - inptr);
	if (inptr && inptr[2] == '?') {
		guchar *decoded;
		gint state = 0;
		gint save = 0;
		gint declen;
		
		d(fprintf (stderr, "encoding is '%c'\n", inptr[0]));
		
		inptr++;
		switch (*inptr) {
		case 'B':
		case 'b':
			inptr += 2;
			decoded = alloca (inend - inptr);
			declen = g_mime_utils_base64_decode_step (inptr, inend - inptr, decoded, &state, &save);
			return g_strndup (decoded, declen);
			break;
		case 'Q':
		case 'q':
			inptr += 2;
			decoded = alloca (inend - inptr);
			declen = quoted_decode (inptr, inend - inptr, decoded);
			
			if (declen == -1) {
				d(fprintf (stderr, "encountered broken 'Q' encoding\n"));
				return NULL;
			}
			
			return g_strndup (decoded, declen);
			break;
		default:
			d(fprintf (stderr, "unknown encoding\n"));
			return NULL;
		}
	}
	
	return NULL;
}


/**
 * g_mime_utils_8bit_header_decode:
 * @in: header to decode
 *
 * Decodes and rfc2047 encoded header.
 *
 * Returns the mime encoded header as 8bit text.
 **/
gchar *
g_mime_utils_8bit_header_decode (const guchar *in)
{
	GString *out, *lwsp, *atom;
	const guchar *inptr;
	guchar *decoded;
	gboolean last_was_encoded = FALSE;
	gboolean last_was_space = FALSE;
	
	out = g_string_sized_new (256);
	lwsp = g_string_sized_new (256);
	atom = g_string_sized_new (256);
	inptr = in;
	
	while (inptr && *inptr) {
		guchar c = *inptr++;
		
		if (!is_atom (c) && !last_was_space) {
			/* we reached the end of an atom */
			gboolean was_encoded;
			guchar *dword = NULL;
			const guchar *word;
			
			if ((was_encoded = is_8bit_word_encoded (atom->str, atom->len)))
				word = dword = decode_encoded_8bit_word (atom->str);
			else
				word = atom->str;
			
			if (word) {
				if (!(last_was_encoded && was_encoded)) {
					/* rfc2047 states that you
                                           must ignore all whitespace
                                           between encoded words */
					g_string_append (out, lwsp->str);
				}
				
				g_string_append (out, word);
				g_free (dword);
			} else {
				was_encoded = FALSE;
				g_string_append (out, lwsp->str);
				g_string_append (out, atom->str);
			}
			
			last_was_encoded = was_encoded;
			
			g_string_truncate (lwsp, 0);
			g_string_truncate (atom, 0);
			
			if (is_lwsp (c)) {
				g_string_append_c (lwsp, c);
				last_was_space = TRUE;
			} else {
				/* This is mostly here for interoperability with broken
                                   mailers that might do something stupid like:
                                   =?iso-8859-1?Q?blah?=:\t=?iso-8859-1?Q?I_am_broken?= */
				g_string_append_c (out, c);
				last_was_encoded = FALSE;
				last_was_space = FALSE;
			}
			
			continue;
		}
		
		if (is_atom (c)) {
			g_string_append_c (atom, c);
			last_was_space = FALSE;
		} else {
			g_string_append_c (lwsp, c);
			last_was_space = TRUE;
		}
	}
	
	if (atom->len || lwsp->len) {
		gboolean was_encoded;
		guchar *dword = NULL;
		const guchar *word;
		
		if ((was_encoded = is_8bit_word_encoded (atom->str, atom->len)))
			word = dword = decode_encoded_8bit_word (atom->str);
		else
			word = atom->str;
		
		if (word) {
			if (!(last_was_encoded && was_encoded)) {
				/* rfc2047 states that you
				   must ignore all whitespace
				   between encoded words */
				g_string_append (out, lwsp->str);
			}
			
			g_string_append (out, word);
			g_free (dword);
		} else {
			g_string_append (out, lwsp->str);
			g_string_append (out, atom->str);
		}
	}
	
	g_string_free (lwsp, TRUE);
	g_string_free (atom, TRUE);
	
	decoded = out->str;
	g_string_free (out, FALSE);
	
	return decoded;
}

/* rfc2047 version of quoted-printable */
static gint
quoted_encode (const guchar *in, gint len, guchar *out, gushort safemask)
{
	register const guchar *inptr, *inend;
	guchar *outptr;
	guchar c;
	
	inptr = in;
	inend = in + len;
	outptr = out;
	
	while (inptr < inend) {
		c = *inptr++;
		if (c == ' ') {
			*outptr++ = '_';
		} else if (gmime_special_table[c] & safemask) {
			*outptr++ = c;
		} else {
			*outptr++ = '=';
			*outptr++ = tohex[(c >> 4) & 0xf];
			*outptr++ = tohex[c & 0xf];
		}
	}
	
	return (outptr - out);
}

static guchar *
encode_8bit_word (const guchar *word, gushort safemask, gboolean *this_was_encoded)
{
	guchar *encoded, *ptr;
	guint enclen, pos, len;
	gint state = 0;
	gint save = 0;
	gchar encoding;
	
	len = strlen (word);
	
	switch (g_mime_utils_best_encoding (word, len)) {
	case GMIME_PART_ENCODING_BASE64:
		enclen = BASE64_ENCODE_LEN (len);
		encoded = alloca (enclen);
		
		encoding = 'b';
		
		pos = g_mime_utils_base64_encode_close (word, len, encoded, &state, &save);
		encoded[pos] = '\0';
		
		/* remove \n chars as headers need to be wrapped differently */
		ptr = encoded;
		while ((ptr = memchr (ptr, '\n', strlen (ptr))))
			memmove (ptr, ptr + 1, strlen (ptr));
		
		break;
	case GMIME_PART_ENCODING_QUOTEDPRINTABLE:
		enclen = QP_ENCODE_LEN (len);
		encoded = alloca (enclen);
		
		encoding = 'q';
		
		pos = quoted_encode (word, len, encoded, safemask);
		encoded[pos] = '\0';
		
		break;
	default:
		if (this_was_encoded)
			*this_was_encoded = FALSE;
		
		return g_strdup (word);
	}
	
	if (this_was_encoded)
		*this_was_encoded = TRUE;
	
	return g_strdup_printf ("=?%s?%c?%s?=", g_mime_charset_locale_name (), encoding, encoded);	
}


/**
 * g_mime_utils_8bit_header_encode_phrase:
 * @in: header to encode
 *
 * Encodes a header phrase according to the rules in rfc2047.
 *
 * Returns the header phrase as 1 encoded atom. Useful for encoding
 * internet addresses.
 **/
gchar *
g_mime_utils_8bit_header_encode_phrase (const guchar *in)
{
	return encode_8bit_word (in, IS_PSAFE, NULL);
}


/**
 * g_mime_utils_8bit_header_encode:
 * @in: header to encode
 *
 * Encodes a header according to the rules in rfc2047.
 *
 * Returns the header as several encoded atoms. Useful for encoding
 * headers like "Subject".
 **/
gchar *
g_mime_utils_8bit_header_encode (const guchar *in)
{
	GString *out, *word, *lwsp;
	guchar *inptr;
	guchar *encoded;
	gboolean is8bit = FALSE;
	gboolean last_was_encoded = FALSE;
	gboolean last_was_space = FALSE;
	
	out = g_string_new ("");
	word = g_string_new ("");
	lwsp = g_string_new ("");
	inptr = (guchar *) in;
	
	while (inptr && *inptr) {
		guchar c = *inptr++;
		
		if (isspace (c) && !last_was_space) {
			gboolean this_was_encoded = FALSE;
			guchar *eword;
			
			if (is8bit)
				eword = encode_8bit_word (word->str, IS_ESAFE, &this_was_encoded);
			else
				eword = g_strdup (word->str);
			
			/* append any whitespace */
			if (last_was_encoded && this_was_encoded) {
				/* we need to encode the whitespace */
				guchar *elwsp;
				gint len;
				
				elwsp = alloca (lwsp->len * 3 + 4);
				len = quoted_encode (lwsp->str, lwsp->len, elwsp, IS_SPACE);
				elwsp[len] = '\0';
				
				g_string_sprintfa (out, " =?%s?q?%s?= ", g_mime_charset_locale_name (), elwsp);
			} else {
				g_string_append (out, lwsp->str);
			}
			
			/* append the encoded word */
			g_string_append (out, eword);
			g_free (eword);
			
			g_string_truncate (lwsp, 0);
			g_string_truncate (word, 0);
			
			last_was_encoded = this_was_encoded;
			is8bit = FALSE;
		}
		
		if (isspace (c)) {
			g_string_append_c (lwsp, c);
			last_was_space = TRUE;
		} else {
			if (c > 127)
				is8bit = TRUE;
			
			g_string_append_c (word, c);
			last_was_space = FALSE;
		}
	}
	
	if (word->len || lwsp->len) {
		gboolean this_was_encoded = FALSE;
		guchar *eword;
		
		if (is8bit)
			eword = encode_8bit_word (word->str, IS_ESAFE, &this_was_encoded);
		else
			eword = g_strdup (word->str);
		
		/* append any whitespace */
		if (last_was_encoded && this_was_encoded) {
			/* we need to encode the whitespace */
			guchar *elwsp;
			gint len;
			
			elwsp = alloca (lwsp->len * 3 + 4);
			len = quoted_encode (lwsp->str, lwsp->len, elwsp, IS_SPACE);
			elwsp[len] = '\0';
			
			g_string_sprintfa (out, " =?%s?q?%s?= ", g_mime_charset_locale_name (), elwsp);
		} else {
			g_string_append (out, lwsp->str);
		}
		
		/* append the encoded word */
		g_string_append (out, eword);
		g_free (eword);
	}
	
	g_string_free (lwsp, TRUE);
	g_string_free (word, TRUE);
	
	encoded = out->str;
	g_string_free (out, FALSE);
	
	return encoded;
}


/**
 * g_mime_utils_base64_encode_close:
 * @in: input stream
 * @inlen: length of the input
 * @out: output string
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 *
 * Base64 encodes the input stream to the output stream. Call this
 * when finished encoding data with g_mime_utils_base64_encode_step to
 * flush off the last little bit.
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_base64_encode_close (const guchar *in, gint inlen, guchar *out, gint *state, guint32 *save)
{
	unsigned char *outptr = out;
	int c1, c2;
	
	if (inlen > 0)
		outptr += g_mime_utils_base64_encode_step (in, inlen, outptr, state, save);
	
	c1 = ((unsigned char *)save)[1];
	c2 = ((unsigned char *)save)[2];
	
	switch (((unsigned char *)save)[0]) {
	case 2:
		outptr[2] = base64_alphabet [(c2 & 0x0f) << 2];
		goto skip;
	case 1:
		outptr[2] = '=';
	skip:
		outptr[0] = base64_alphabet [c1 >> 2];
		outptr[1] = base64_alphabet [c2 >> 4 | ((c1 & 0x3) << 4)];
		outptr[3] = '=';
		outptr += 4;
		break;
	}
	*outptr++ = '\n';
	
	*save = 0;
	*state = 0;
	
	return (outptr - out);
}


/**
 * g_mime_utils_base64_encode_step:
 * @in: input stream
 * @inlen: length of the input
 * @out: output string
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 *
 * Base64 encodes a chunk of data. Performs an 'encode step', only
 * encodes blocks of 3 characters to the output at a time, saves
 * left-over state in state and save (initialise to 0 on first
 * invocation).
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_base64_encode_step (const guchar *in, gint inlen, guchar *out, gint *state, guint32 *save)
{
	const register unsigned char *inptr;
	register unsigned char *outptr;
	
	if (inlen <= 0)
		return 0;
	
	inptr = in;
	outptr = out;
	
	d(printf("we have %d chars, and %d saved chars\n", inlen, ((gchar *)save)[0]));
	
	if (inlen + ((unsigned char *)save)[0] > 2) {
		const unsigned char *inend = in + inlen - 2;
		register int c1 = 0, c2 = 0, c3 = 0;
		register int already;
		
		already = *state;
		
		switch (((gchar *)save)[0]) {
		case 1:	c1 = ((unsigned char *)save)[1]; goto skip1;
		case 2:	c1 = ((unsigned char *)save)[1];
			c2 = ((unsigned char *)save)[2]; goto skip2;
		}
		
		/* yes, we jump into the loop, no i'm not going to change it, its beautiful! */
		while (inptr < inend) {
			c1 = *inptr++;
		skip1:
			c2 = *inptr++;
		skip2:
			c3 = *inptr++;
			*outptr++ = base64_alphabet [c1 >> 2];
			*outptr++ = base64_alphabet [(c2 >> 4) | ((c1 & 0x3) << 4)];
			*outptr++ = base64_alphabet [((c2 & 0x0f) << 2) | (c3 >> 6)];
			*outptr++ = base64_alphabet [c3 & 0x3f];
			/* this is a bit ugly ... */
			if ((++already) >= 19) {
				*outptr++ = '\n';
				already = 0;
			}
		}
		
		((unsigned char *)save)[0] = 0;
		inlen = 2 - (inptr - inend);
		*state = already;
	}
	
	d(printf ("state = %d, inlen = %d\n", (gint)((gchar *)save)[0], inlen));
	
	if (inlen > 0) {
		register gchar *saveout;
		
		/* points to the slot for the next char to save */
		saveout = & (((gchar *)save)[1]) + ((gchar *)save)[0];
		
		/* inlen can only be 0 1 or 2 */
		switch (inlen) {
		case 2:	*saveout++ = *inptr++;
		case 1:	*saveout++ = *inptr++;
		}
		((gchar *)save)[0] += inlen;
	}
	
	d(printf ("mode = %d\nc1 = %c\nc2 = %c\n",
		  (gint)((gchar *)save)[0],
		  (gint)((gchar *)save)[1],
		  (gint)((gchar *)save)[2]));
	
	return (outptr - out);
}

/**
 * g_mime_utils_base64_decode_step:
 * @in: input stream
 * @inlen: max length of data to decode
 * @out: output stream
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been decoded
 *
 * Decodes a chunk of base64 encoded data.
 *
 * Returns the number of bytes decoded (which have been dumped in @out).
 **/
gint
g_mime_utils_base64_decode_step (const guchar *in, gint inlen, guchar *out, gint *state, guint32 *save)
{
	const register unsigned char *inptr;
	register unsigned char *outptr;
	const unsigned char *inend;
	register guint32 saved;
	unsigned char c;
	int i;
	
	inend = in + inlen;
	outptr = out;
	
	/* convert 4 base64 bytes to 3 normal bytes */
	saved = *save;
	i = *state;
	inptr = in;
	while (inptr < inend) {
		c = gmime_base64_rank[*inptr++];
		if (c != 0xff) {
			saved = (saved << 6) | c;
			i++;
			if (i == 4) {
				*outptr++ = saved >> 16;
				*outptr++ = saved >> 8;
				*outptr++ = saved;
				i = 0;
			}
		}
	}
	
	*save = saved;
	*state = i;
	
	/* quick scan back for '=' on the end somewhere */
	/* fortunately we can drop 1 output char for each trailing = (upto 2) */
	i = 2;
	while (inptr > in && i) {
		inptr--;
		if (gmime_base64_rank[*inptr] != 0xff) {
			if (*inptr == '=')
				outptr--;
			i--;
		}
	}
	
	/* if i != 0 then there is a truncation error! */
	return (outptr - out);
}


/**
 * g_mime_utils_uuencode_close:
 * @in: input stream
 * @inlen: input stream length
 * @out: output stream
 * @uubuf: temporary buffer of 60 bytes
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 * @uulen: holds the value of the length-char which is used to calculate
 *         how many more chars need to be decoded for that 'line'
 *
 * Uuencodes a chunk of data. Call this when finished encoding data
 * with g_mime_utils_uuencode_step to flush off the last little bit.
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_uuencode_close (const guchar *in, gint inlen, guchar *out, guchar *uubuf, gint *state, guint32 *save, gchar *uulen)
{
	register unsigned char *outptr, *bufptr;
	register guint32 saved;
	int i;
	
	outptr = out;
	
	if (inlen > 0)
		outptr += g_mime_utils_uuencode_step (in, inlen, out, uubuf, state, save, uulen);
	
	bufptr = uubuf + ((*uulen / 3) * 4);
	saved = *save;
	i = *state;
	
	if (i > 0) {
		while (i < 3) {
			saved <<= 8 | 0;
			i++;
		}
		
		if (i == 3) {
			/* convert 3 normal bytes into 4 uuencoded bytes */
			unsigned char b0, b1, b2;
			
			b0 = saved >> 16;
			b1 = saved >> 8 & 0xff;
			b2 = saved & 0xff;
			
			*bufptr++ = GMIME_UUENCODE_CHAR ((b0 >> 2) & 0x3f);
			*bufptr++ = GMIME_UUENCODE_CHAR (((b0 << 4) | ((b1 >> 4) & 0xf)) & 0x3f);
			*bufptr++ = GMIME_UUENCODE_CHAR (((b1 << 2) | ((b2 >> 6) & 0x3)) & 0x3f);
			*bufptr++ = GMIME_UUENCODE_CHAR (b2 & 0x3f);
		}
	}
	
	if (*uulen || *state) {
		int cplen = (((*uulen + (*state ? 3 : 0)) / 3) * 4);
		
		*outptr++ = GMIME_UUENCODE_CHAR (*uulen + *state);
		memcpy (outptr, uubuf, cplen);
		outptr += cplen;
		*outptr++ = '\n';
		*uulen = 0;
	}
	
	*outptr++ = GMIME_UUENCODE_CHAR (*uulen);
	*outptr++ = '\n';
	
	*save = 0;
	*state = 0;
	
	return (outptr - out);
}


/**
 * g_mime_utils_uuencode_step:
 * @in: input stream
 * @inlen: input stream length
 * @out: output stream
 * @uubuf: temporary buffer of 60 bytes
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 * @uulen: holds the value of the length-char which is used to calculate
 *         how many more chars need to be decoded for that 'line'
 *
 * Uuencodes a chunk of data. Performs an 'encode step', only encodes
 * blocks of 45 characters to the output at a time, saves left-over
 * state in @uubuf, @state and @save (initialize to 0 on first
 * invocation).
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_uuencode_step (const guchar *in, gint inlen, guchar *out, guchar *uubuf, gint *state, guint32 *save, gchar *uulen)
{
	register unsigned char *outptr, *bufptr;
	const register unsigned char *inptr;
	const unsigned char *inend;
	register guint32 saved;
	int i;
	
	if (*uulen <= 0)
		*uulen = 0;
	
	inptr = in;
	inend = in + inlen;
	
	outptr = out;
	
	bufptr = uubuf + ((*uulen / 3) * 4);
	
	saved = *save;
	i = *state;
	
	while (inptr < inend) {
		while (*uulen < 45 && inptr < inend) {
			while (i < 3 && inptr < inend) {
				saved = (saved << 8) | *inptr++;
				i++;
			}
			
			if (i == 3) {
				/* convert 3 normal bytes into 4 uuencoded bytes */
				unsigned char b0, b1, b2;
				
				b0 = saved >> 16;
				b1 = saved >> 8 & 0xff;
				b2 = saved & 0xff;
				
				*bufptr++ = GMIME_UUENCODE_CHAR ((b0 >> 2) & 0x3f);
				*bufptr++ = GMIME_UUENCODE_CHAR (((b0 << 4) | ((b1 >> 4) & 0xf)) & 0x3f);
				*bufptr++ = GMIME_UUENCODE_CHAR (((b1 << 2) | ((b2 >> 6) & 0x3)) & 0x3f);
				*bufptr++ = GMIME_UUENCODE_CHAR (b2 & 0x3f);
				
				i = 0;
				saved = 0;
				*uulen += 3;
			}
		}
		
		if (*uulen >= 45) {
			*outptr++ = GMIME_UUENCODE_CHAR (*uulen);
			memcpy (outptr, uubuf, ((*uulen / 3) * 4));
			outptr += ((*uulen / 3) * 4);
			*outptr++ = '\n';
			*uulen = 0;
			bufptr = uubuf;
		}
	}
	
	*save = saved;
	*state = i;
	
	return (outptr - out);
}


/**
 * g_mime_utils_uudecode_step:
 * @in: input stream
 * @inlen: max length of data to decode (normally strlen (in) ??)
 * @out: output stream
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been decoded
 * @uulen: holds the value of the length-char which is used to calculate
 *         how many more chars need to be decoded for that 'line'
 *
 * Uudecodes a chunk of data. Performs a 'decode step' on a chunk of
 * uuencoded data. Assumes the "begin <mode> <file name>" line has
 * been stripped off.
 *
 * Returns the number of bytes decoded.
 **/
gint
g_mime_utils_uudecode_step (const guchar *in, gint inlen, guchar *out, gint *state, guint32 *save, gchar *uulen)
{
	const register unsigned char *inptr;
	register unsigned char *outptr;
	const unsigned char *inend;
	unsigned char ch;
	register guint32 saved;
	gboolean last_was_eoln;
	int i;
	
	if (*uulen <= 0)
		last_was_eoln = TRUE;
	else
		last_was_eoln = FALSE;
	
	inend = in + inlen;
	outptr = out;
	saved = *save;
	i = *state;
	inptr = in;
	while (inptr < inend && *inptr) {
		if (*inptr == '\n' || last_was_eoln) {
			if (last_was_eoln) {
				*uulen = gmime_uu_rank[*inptr];
				last_was_eoln = FALSE;
			} else {
				last_was_eoln = TRUE;
			}
			
			inptr++;
			continue;
		}
		
		ch = *inptr++;
		
		if (*uulen > 0) {
			/* save the byte */
			saved = (saved << 8) | ch;
			i++;
			if (i == 4) {
				/* convert 4 uuencoded bytes to 3 normal bytes */
				unsigned char b0, b1, b2, b3;
				
				b0 = saved >> 24;
				b1 = saved >> 16 & 0xff;
				b2 = saved >> 8 & 0xff;
				b3 = saved & 0xff;
				
				if (*uulen >= 3) {
					*outptr++ = gmime_uu_rank[b0] << 2 | gmime_uu_rank[b1] >> 4;
					*outptr++ = gmime_uu_rank[b1] << 4 | gmime_uu_rank[b2] >> 2;
				        *outptr++ = gmime_uu_rank[b2] << 6 | gmime_uu_rank[b3];
				} else {
					if (*uulen >= 1) {
						*outptr++ = gmime_uu_rank[b0] << 2 | gmime_uu_rank[b1] >> 4;
					}
					if (*uulen >= 2) {
						*outptr++ = gmime_uu_rank[b1] << 4 | gmime_uu_rank[b2] >> 2;
					}
				}
				
				i = 0;
				saved = 0;
				*uulen -= 3;
			}
		} else {
			break;
		}
	}
	
	*save = saved;
	*state = i;
	
	return (outptr - out);
}


/**
 * g_mime_utils_quoted_encode_close:
 * @in: input stream
 * @inlen: length of the input
 * @out: output string
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 *
 * Quoted-printable encodes a block of text. Call this when finished
 * encoding data with g_mime_utils_quoted_encode_step to flush off the
 * last little bit.
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_quoted_encode_close (const guchar *in, gint inlen, guchar *out, gint *state, gint *save)
{
	register unsigned char *outptr = out;
	int last;
	
	if (inlen > 0)
		outptr += g_mime_utils_quoted_encode_step (in, inlen, outptr, state, save);
	
	last = *state;
	if (last != -1) {
		/* space/tab must be encoded if its the last character on
		   the line */
		if (is_qpsafe (last) && !isblank (last)) {
			*outptr++ = last;
		} else {
			*outptr++ = '=';
			*outptr++ = tohex[(last >> 4) & 0xf];
			*outptr++ = tohex[last & 0xf];
		}
	}
	
	*outptr++ = '\n';
	
	*save = 0;
	*state = -1;
	
	return (outptr - out);
}


/**
 * g_mime_utils_quoted_encode_step:
 * @in: input stream
 * @inlen: length of the input
 * @out: output string
 * @state: holds the number of bits that are stored in @save
 * @save: leftover bits that have not yet been encoded
 *
 * Quoted-printable encodes a block of text. Performs an 'encode
 * step', saves left-over state in state and save (initialise to -1 on
 * first invocation).
 *
 * Returns the number of bytes encoded.
 **/
gint
g_mime_utils_quoted_encode_step (const guchar *in, gint inlen, guchar *out, gint *state, gint *save)
{
	const register unsigned char *inptr, *inend;
	register unsigned char *outptr;
	unsigned char c;
	register int sofar = *save;  /* keeps track of how many chars on a line */
	register int last = *state;  /* keeps track if last char to end was a space cr etc */
	
	inptr = in;
	inend = in + inlen;
	outptr = out;
	while (inptr < inend) {
		c = *inptr++;
		if (c == '\r') {
			if (last != -1) {
				*outptr++ = '=';
				*outptr++ = tohex[(last >> 4) & 0xf];
				*outptr++ = tohex[last & 0xf];
				sofar += 3;
			}
			last = c;
		} else if (c == '\n') {
			if (last != -1 && last != '\r') {
				*outptr++ = '=';
				*outptr++ = tohex[(last >> 4) & 0xf];
				*outptr++ = tohex[last & 0xf];
			}
			*outptr++ = '\n';
			sofar = 0;
			last = -1;
		} else {
			if (last != -1) {
				if (is_qpsafe (last)) {
					*outptr++ = last;
					sofar++;
				} else {
					*outptr++ = '=';
					*outptr++ = tohex[(last >> 4) & 0xf];
					*outptr++ = tohex[last & 0xf];
					sofar += 3;
				}
			}
			
			if (is_qpsafe (c)) {
				if (sofar > 74) {
					*outptr++ = '=';
					*outptr++ = '\n';
					sofar = 0;
				}
				
				/* delay output of space char */
				if (isblank (c)) {
					last = c;
				} else {
					*outptr++ = c;
					sofar++;
					last = -1;
				}
			} else {
				if (sofar > 72) {
					*outptr++ = '=';
					*outptr++ = '\n';
					sofar = 3;
				} else
					sofar += 3;
				
				*outptr++ = '=';
				*outptr++ = tohex[(c >> 4) & 0xf];
				*outptr++ = tohex[c & 0xf];
				last = -1;
			}
		}
	}
	*save = sofar;
	*state = last;
	
	return (outptr - out);
}


/**
 * g_mime_utils_quoted_decode_step: decode a chunk of QP encoded data
 * @in: input stream
 * @inlen: max length of data to decode
 * @out: output stream
 * @savestate: holds the number of bits that are stored in @save
 * @saved: leftover bits that have not yet been decoded
 *
 * Decodes a block of quoted-printable encoded data. Performs a
 * 'decode step' on a chunk of QP encoded data.
 *
 * Returns the number of bytes decoded.
 **/
gint
g_mime_utils_quoted_decode_step (const guchar *in, gint inlen, guchar *out, gint *savestate, gint *saved)
{
	/* FIXME: this does not strip trailing spaces from lines (as
	 * it should, rfc 2045, section 6.7) Should it also
	 * canonicalise the end of line to CR LF??
	 *
	 * Note: Trailing rubbish (at the end of input), like = or =x
	 * or =\r will be lost.
	 */
	const register unsigned char *inptr;
	register unsigned char *outptr;
	const unsigned char *inend;
	unsigned char c;
	int state, save;
	
	inend = in + inlen;
	outptr = out;
	
	d(printf ("quoted-printable, decoding text '%.*s'\n", inlen, in));
	
	state = *savestate;
	save = *saved;
	inptr = in;
	while (inptr < inend) {
		switch (state) {
		case 0:
			while (inptr < inend) {
				c = *inptr++;
				/* FIXME: use a specials table to avoid 3 comparisons for the common case */
				if (c == '=') { 
					state = 1;
					break;
				}
#ifdef CANONICALISE_EOL
				/*else if (c=='\r') {
					state = 3;
				} else if (c=='\n') {
					*outptr++ = '\r';
					*outptr++ = c;
					} */
#endif
				else {
					*outptr++ = c;
				}
			}
			break;
		case 1:
			c = *inptr++;
			if (c == '\n') {
				/* soft break ... unix end of line */
				state = 0;
			} else {
				save = c;
				state = 2;
			}
			break;
		case 2:
			c = *inptr++;
			if (isxdigit (c) && isxdigit (save)) {
				c = toupper (c);
				save = toupper (save);
				*outptr++ = (((save >= 'A' ? save - 'A' + 10 : save - '0') & 0x0f) << 4)
					| ((c >= 'A' ? c - 'A' + 10 : c - '0') & 0x0f);
			} else if (c == '\n' && save == '\r') {
				/* soft break ... canonical end of line */
			} else {
				/* just output the data */
				*outptr++ = '=';
				*outptr++ = save;
				*outptr++ = c;
			}
			state = 0;
			break;
#ifdef CANONICALISE_EOL
		case 3:
			/* convert \n -> to \r\n, leaves \r\n alone */
			c = *inptr++;
			if (c == '\n') {
				*outptr++ = '\r';
				*outptr++ = c;
			} else {
				*outptr++ = '\r';
				*outptr++ = '\n';
				*outptr++ = c;
			}
			state = 0;
			break;
#endif
		}
	}
	
	*savestate = state;
	*saved = save;
	
	return (outptr - out);
}
