/*
 * xmlformat - registration of xml object formats 
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

#ifndef XMLFORMAT_COMMON_H
#define XMLFORMAT_COMMON_H

// Categories
#define XMLFORMAT_COMMON_CATEGORIES			"Categories"
#define XMLFORMAT_COMMON_CATEGORIES_CATEGORY		"Category"

// Class
#define XMLFORMAT_COMMON_CLASS				"Class"
#define XMLFORMAT_COMMON_CONTENT			"Content"
#define XMLFORMAT_COMMON_CONTENT_PUBLIC			"PUBLIC"
#define XMLFORMAT_COMMON_CONTENT_PRIVATE		"PRIVATE"
#define XMLFORMAT_COMMON_CONTENT_CONFIDENTIAL		"CONFIDENTIAL"
#define XMLFORMAT_COMMON_CONTENT_DEFAULT		XMLFORMAT_COMMON_CONTENT_PUBLIC	

// DateValueType
#define XMLFORMAT_COMMON_DATEVALUETYPE			"DateValueType"
#define XMLFORMAT_COMMON_DATEVALUETYPE_DATE		"DATE"
#define XMLFORMAT_COMMON_DATEVALUETYPE_DATETIME		"DATE-TIME"

// DateTimeContent
#define XMLFORMAT_COMMON_DATETIMECONTENT		"DateTimeContent"
#define XMLFORMAT_COMMON_DATETIMECONTENT_CONTENT	"Content"
#define XMLFORMAT_COMMON_DATETIMECONTENT_VALUE		"Value"
#define XMLFORMAT_COMMON_DATETIMECONTENT_TIMEZONEID	"TimezoneID"

// Multitext
#define XMLFORMAT_COMMON_MULTITEXT			"MultiText"
#define XMLFORMAT_COMMON_MULTITEXT_CONTENT		"Content"
#define XMLFORMAT_COMMON_MULTITEXT_ALTERNATIVETEXTREP	"AlternativeTextRep"
#define XMLFORMAT_COMMON_MULTITEXT_LANGUAGE		"Language"

// StringContent
#define XMLFORMAT_COMMON_STRINGCONTENT			"StringContent"
#define XMLFORMAT_COMMON_STRINGCONTENT_CONTENT		"Content"

#endif /* XMLFORMAT_COMMON_H */
