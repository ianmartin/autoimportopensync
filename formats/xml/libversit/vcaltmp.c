/*
This module provides some helper APIs for creating
a VCalendar object.

Note on APIs:
	1.	The APIs does not attempt to verify if the arguments
		passed are correct.
	2.	Where the argument to an API is not applicable, pass
		the value 0.
	3.	See the test program at the bottom of this file as an
		example of usage.
	4.	This code calls APIs in vobject.c. 

*/

/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/


#include <stdio.h>
#include <string.h>
#include "vcaltmp.h"

	
DLLEXPORT(VObjectO*) vcsCreateVCalO(
	char *date_created,
	char *location,
	char *product_id,
	char *time_zone,
	char *version
	)
    {
    VObjectO *vcal = newVObjectO(VCCalPropO);
#define Z(p,v) if (v) addPropValueO(vcal,p,v);
    Z(VCDCreatedPropO, date_created);
    Z(VCLocationPropO, location)
    Z(VCProdIdPropO, product_id)
    Z(VCTimeZonePropO, time_zone)
    Z(VCVersionPropO, version)
#undef Z
    return vcal;
    }


DLLEXPORT(VObjectO*) vcsAddEventO(
	VObjectO *vcal,
	char *start_date_time,
	char *end_date_time,
	char *description,
	char *summary,
	char *categories,
	char *classification,
	char *status,
	char *transparency,
	char *uid,
	char *url
	)
    {
    VObjectO *vevent = addPropO(vcal,VCEventPropO);
#define Z(p,v) if (v) addPropValueO(vevent,p,v);
    Z(VCDTstartPropO,start_date_time);
    Z(VCDTendPropO,end_date_time);
    if (description) {
	VObjectO *p = addPropValueO(vevent,VCDescriptionPropO,description);
	if (strchr(description,'\n'))
	    addPropO(p,VCQuotedPrintablePropO);
	}
    Z(VCSummaryPropO,summary);
    Z(VCCategoriesPropO,categories);
    Z(VCClassPropO,classification);
    Z(VCStatusPropO,status);
    Z(VCTranspPropO,transparency);
    Z(VCUniqueStringPropO,uid);
    Z(VCURLPropO,url);
#undef Z
    return vevent;
    }


DLLEXPORT(VObjectO*) vcsAddTodoO(
	VObjectO *vcal,
	char *start_date_time,
	char *due_date_time,
	char *date_time_complete,
	char *description,
	char *summary,
	char *priority,
	char *classification,
	char *status,
	char *uid,
	char *url
	)
    {
    VObjectO *vtodo = addPropO(vcal,VCTodoPropO);
#define Z(p,v) if (v) addPropValueO(vtodo,p,v);
    Z(VCDTstartPropO,start_date_time);
    Z(VCDuePropO,due_date_time);
    Z(VCCompletedPropO,date_time_complete);
    if (description) {
	VObjectO *p = addPropValueO(vtodo,VCDescriptionPropO,description);
	if (strchr(description,'\n'))
	    addPropO(p,VCQuotedPrintablePropO);
	}
    Z(VCSummaryPropO,summary);
    Z(VCPriorityPropO,priority);
    Z(VCClassPropO,classification);
    Z(VCStatusPropO,status);
    Z(VCUniqueStringPropO,uid);
    Z(VCURLPropO,url);
#undef Z
    return vtodo;
    }


DLLEXPORT(VObjectO*) vcsAddAAlarmO(
	VObjectO *vevent,
	char *run_time,
	char *snooze_time,
	char *repeat_count,
	char *audio_content
	)
    {
    VObjectO *aalarm= addPropO(vevent,VCAAlarmPropO);
#define Z(p,v) if (v) addPropValueO(aalarm,p,v);
    Z(VCRunTimePropO,run_time);
    Z(VCSnoozeTimePropO,snooze_time);
    Z(VCRepeatCountPropO,repeat_count);
    Z(VCAudioContentPropO,audio_content);
#undef Z
    return aalarm;
    }


DLLEXPORT(VObjectO*) vcsAddMAlarmO(
	VObjectO *vevent,
	char *run_time,
	char *snooze_time,
	char *repeat_count,
	char *email_address,
	char *note
	)
    {
    VObjectO *malarm= addPropO(vevent,VCMAlarmPropO);
#define Z(p,v) if (v) addPropValueO(malarm,p,v);
    Z(VCRunTimePropO,run_time);
    Z(VCSnoozeTimePropO,snooze_time);
    Z(VCRepeatCountPropO,repeat_count);
    Z(VCEmailAddressPropO,email_address);
    Z(VCNotePropO,note);
#undef Z
    return malarm;
    }


DLLEXPORT(VObjectO*) vcsAddDAlarmO(
	VObjectO *vevent,
	char *run_time,
	char *snooze_time,
	char *repeat_count,
	char *display_string
	)
    {
    VObjectO *dalarm= addPropO(vevent,VCDAlarmPropO);
#define Z(p,v) if (v) addPropValueO(dalarm,p,v);
    Z(VCRunTimePropO,run_time);
    Z(VCSnoozeTimePropO,snooze_time);
    Z(VCRepeatCountPropO,repeat_count);
    Z(VCDisplayStringPropO,display_string);
#undef Z
    return dalarm;
    }


DLLEXPORT(VObjectO*) vcsAddPAlarmO(
	VObjectO *vevent,
	char *run_time,
	char *snooze_time,
	char *repeat_count,
	char *procedure_name
	)
    {
    VObjectO *palarm= addPropO(vevent,VCPAlarmPropO);
#define Z(p,v) if (v) addPropValueO(palarm,p,v);
    Z(VCRunTimePropO,run_time);
    Z(VCSnoozeTimePropO,snooze_time);
    Z(VCRepeatCountPropO,repeat_count);
    Z(VCProcedureNamePropO,procedure_name);
#undef Z
    return palarm;
    }

    
#ifdef _TEST

#if 0
This testcase would generate a file call "frankcal.vcf" with
the following content:

BEGIN:VCALENDAR
DCREATED:19960523T100522
GEO:37.24,-17.87
PRODID:-//Frank Dawson/Hand Crafted In North Carolina//NONSGML Made By Hand//EN
VERSION:0.3
BEGIN:VEVENT
DTSTART:19960523T120000
DTEND:19960523T130000
DESCRIPTION;QUOTED-PRINTABLE:VERSIT PDI PR Teleconference/Interview =0A=
With Tom Streeter and Frank Dawson - Discuss VERSIT PDI project and vCard and vCalendar=0A=
activities with European Press representatives.
SUMMARY:VERSIT PDI PR Teleconference/Interview
CATEGORIES:PHONE CALL
STATUS:CONFIRMED
TRANSP:19960523T100522-4000F100582713-009251
UID:http://www.ibm.com/raleigh/fdawson/~c:\or2\orgfiles\versit.or2
DALARM:19960523T114500;5;3;Your Telecon Starts At Noon!!!;
MALARM:19960522T120000;;;fdawson@raleigh.ibm.com;Remember 05/23 Noon Telecon!!!;
PALARM:19960523T115500;;;c:\or2\organize.exe c:\or2\orgfiles\versit.or2;
X-LDC-OR2-OLE:c:\temp\agenda.doc
END:VEVENT

BEGIN:VTODO
DUE:19960614T0173000
DESCRIPTION:Review VCalendar helper API.
END:VTODO

END:VCALENDAR

#endif

void testVcalAPIs() {
    FILE *fp;
    VObjectO *vcal = vcsCreateVCalO(
	"19960523T100522",
	"37.24,-17.87",
	"-//Frank Dawson/Hand Crafted In North Carolina//NONSGML Made By Hand//EN",
	0,
	"0.3"
	);

    VObjectO *vevent = vcsAddEventO(
	vcal,
	"19960523T120000",
	"19960523T130000",
	"VERSIT PDI PR Teleconference/Interview \nWith Tom Streeter and Frank Dawson - Discuss VERSIT PDI project and vCard and vCalendar\nactivities with European Press representatives.",
	"VERSIT PDI PR Teleconference/Interview",
	"PHONE CALL",
	0,
	"CONFIRMED",
	"19960523T100522-4000F100582713-009251",
	"http://www.ibm.com/raleigh/fdawson/~c:\\or2\\orgfiles\\versit.or2",
	0
	);
    
    vcsAddDAlarmO(vevent, "19960523T114500", "5", "3",
	    "Your Telecon Starts At Noon!!!");
    vcsAddMAlarmO(vevent, "19960522T120000", 0, 0, "fdawson@raleigh.ibm.com",
	    "Remember 05/23 Noon Telecon!!!");
    vcsAddPAlarmO(vevent, "19960523T115500", 0 ,0,
	    "c:\\or2\\organize.exe c:\\or2\\orgfiles\\versit.or2");
    
    addPropValueO(vevent, "X-LDC-OR2-OLE", "c:\\temp\\agenda.doc");

    vcsAddTodoO(
	vcal,
	0,
	"19960614T0173000",
	0,
	"Review VCalendar helper API.",
	0,
	0,
	0,
	0,
	0,
	0
	);

    /* now do something to the resulting VObjectO */
    /* pretty print on stdout for fun */
    printVObjectO(vcal);
    /* open the output text file */

#define OUTFILE "frankcal.vcf"

    fp = fopen(OUTFILE, "w");
    if (fp) {
	/* write it in text form */
	writeVObjectO(fp,vcal);
	fclose(fp);
	}
    else {
	printf("open output file '%s' failed\n", OUTFILE);
	}
    }

void main() {
    testVcalAPIs();
    }

#endif


/* end of source file vcaltmp.c */
