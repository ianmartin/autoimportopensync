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

/*
 * This software is based on the C++ Portable Types Library (PTypes) by
 * Hovik Melikyan
 *
 *  C++ Portable Types Library (PTypes)
 *  Version 1.8.3  Released 25-Aug-2003
 *
 *  Copyright (c) 2001, 2002, 2003 Hovik Melikyan
 *
 *  http://www.melikyan.com/ptypes/
 *  http://ptypes.sourceforge.net/
 *
 */
#include "unixdefs.h"
 
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32_WCE
#   include <wchar.h>
#endif

#ifdef _WIN32_WCE
#   include "ptypes/wmem.h"
#endif
#include "ptypes/ptypes.h"


PTYPES_BEGIN


void string::initialize(const wchar_t* s1, int len1, const wchar_t* s2, int len2)
{
    if (len1 <= 0)
        initialize(s2, len2);
    else if (len2 <= 0)
        initialize(s1, len1);
    else
    {
        _alloc((len1 + len2));
        wmemcpy(data, s1, len1);
        wmemcpy(data + len1, s2, len2);
    }
}


#ifdef CHECK_BOUNDS

wchar_t& string::operator[] (int i)
{
    unique(*this);
    if (i < 0 || i >= length(*this))
        fatal(CRIT_FIRST + 20, "String index overflow");
    return data[i];
}

#endif


void concat(string& s, const wchar_t* sc, int catlen)
{
    if (length(s) == 0)
        s.assign(sc, catlen);
    else if (catlen > 0)
    {
        int oldlen = length(s);

        // we must check this before calling setlength(), since
        // the buffer pointer may be changed during reallocation
        if (s.data == sc)
        {
            setlength(s, oldlen + catlen);
            wmemmove(s.data + oldlen, s.data, catlen);
        }
        else
        {
            setlength(s, oldlen + catlen);
            wmemmove(s.data + oldlen, sc, catlen);
        }
    }
}


void concat(string& s, const wchar_t* sc)
{
    concat(s, sc, hstrlen(sc));
}


void concat(string& s, wchar_t c)
{
    if (length(s) == 0)
        s.assign(c);
    else
    {
        setlength(s, length(s) + 1);
        s.data[length(s) - 1] = c;
    }
}


void concat(string& s, const string& s1)
{
    if (length(s) == 0)
        s = s1;
    else if (length(s1) > 0)
        concat(s, s1.data, length(s1));
}


bool contains(const wchar_t* s1, int s1len, const string& s, int at)
{
    return (s1len > 0) && (at >= 0) && (at + s1len <= length(s))
        && (wmemcmp(s.data + at, s1, s1len) == 0);
}


bool contains(const wchar_t* s1, const string& s, int at)
{
    return contains(s1, hstrlen(s1), s, at);
}


bool contains(wchar_t s1, const string& s, int at)
{
    return (at >= 0) && (at < length(s)) && (s.data[at] == s1);
}


bool contains(const string& s1, const string& s, int at)
{
    return contains(s1.data, length(s1), s, at);
}


string string::operator+ (const wchar_t* sc) const
{
    if (length(*this) == 0)
        return string(sc);
    else
        return string(data, length(*this), sc, hstrlen(sc));
}


string string::operator+ (wchar_t c) const
{
    if (length(*this) == 0)
        return string(c);
    else
        return string(data, length(*this), &c, 1);
}


string string::operator+ (const string& s) const
{
    if (length(*this) == 0)
        return s;
    else if (length(s) == 0)
        return *this;
    else
        return string(data, length(*this), s.data, length(s));
}


string operator+ (const wchar_t* sc, const string& s)
{
    if (length(s) == 0)
        return string(sc);
    else
        return string(sc, hstrlen(sc), s.data, length(s));
}


string operator+ (wchar_t c, const string& s)
{
    if (length(s) == 0)
        return string(c);
    else
        return string(&c, 1, s.data, length(s));
}


bool string::operator== (const string& s) const
{
    return (length(*this) == length(s))
        && ((length(*this) == 0) || (wmemcmp(data, s.data, length(*this)) == 0));
}


bool string::operator== (wchar_t c) const
{
    return (length(*this) == 1) && (data[0] == c);
}


string copy(const string& s, int from, int cnt)
{
    string t;
    if (length(s) > 0 && from >= 0 && from < length(s))
    {
        int l = imin(cnt, length(s) - from);
        if (from == 0 && l == length(s))
            t = s;
        else if (l > 0)
        {
            t._alloc(l);
            wmemmove(t.data, s.data + from, l);
            t.data[l] = 0;
        }
    }
    return t;
}


void ins(const wchar_t* s1, int s1len, string& s, int at)
{
    int curlen = length(s);
    if (s1len > 0 && at >= 0 && at < curlen)
    {
        if (curlen == 0)
            s.assign(s1, s1len);
        else
        {
            setlength(s, curlen + s1len);
            int t = length(s) - at - s1len;
            wchar_t* p = s.data + at;
            if (t > 0)
                wmemmove(p + s1len, p, t);
            wmemmove(p, s1, s1len);
        }
    }
}


void ins(const wchar_t* sc, string& s, int at)
{
    ins(sc, hstrlen(sc), s, at);
}


void ins(wchar_t c, string& s, int at)
{
    ins(&c, 1, s, at);
}


void ins(const string& s1, string& s, int at)
{
    ins(s1.data, length(s1), s, at);
}


void del(string& s, int from, int cnt)
{
    int l = length(s);
    if (from >= 0 && from < l && cnt > 0)
    {
        if (from + cnt >= l)
            cnt = l - from;
        else if (l - from > cnt)
        {
            unique(s);
            wmemmove(s.data + from, s.data + from + cnt, (l - from - cnt));
        }
        setlength(s, l - cnt);
    }
}


int pos(const wchar_t* sc, const string& s)
{
    const wchar_t* t = (wchar_t*)wcsstr(s.data, sc);
    return (t == NULL ? (-1) : (t - s.data));
}


int pos(wchar_t c, const string& s)
{
    const wchar_t* t = (wchar_t*)wcschr(s.data, c);
    return (t == NULL ? (-1) : (t - s.data));
}


int rpos(wchar_t c, const string& s)
{
    const wchar_t* t = (wchar_t*)wcsrchr(s.data, c);
    return (t == NULL ? (-1) : (t - s.data));
}


PTYPES_END


