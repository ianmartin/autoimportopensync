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

/*
 
The vCard/vCalendar C interface is implemented in the set 
of files as follows:

vcc.y, yacc source, and vcc.c, the yacc output you will use
implements the core parser

vobject.c implements an API that insulates the caller from
the parser and changes in the vCard/vCalendar BNF

port.h defines compilation environment dependent stuff

vcc.h and vobject.h are header files for their .c counterparts

vcaltmp.h and vcaltmp.c implement vCalendar "macro" functions
which you may find useful.

test.c is a standalone test driver that exercises some of
the features of the APIs provided. Invoke test.exe on a
VCARD/VCALENDAR input text file and you will see the pretty
print output of the internal representation (this pretty print
output should give you a good idea of how the internal 
representation looks like -- there is one such output in the 
following too). Also, a file with the .out suffix is generated 
to show that the internal representation can be written back 
in the original text format.

For more information on this API see the readme.txt file
which accompanied this distribution.

  Also visit:

		http://www.versit.com
		http://www.ralden.com

*/


#ifndef __VOBJECT_H__
#define __VOBJECT_H__ 1


#include "port.h"
#include <stdlib.h>
#include <stdio.h>

#if defined(__CPLUSPLUS__) || defined(__cplusplus)
extern "C" {
#endif


#define VC7bitPropO				"7BIT"
#define VC8bitPropO				"8BIT"
#define VCAAlarmPropO			"AALARM"
#define VCAdditionalNamesPropO	"ADDN"
#define VCAdrPropO				"ADR"
#define VCAgentPropO				"AGENT"
#define VCAIFFPropO				"AIFF"
#define VCAOLPropO				"AOL"
#define VCAppleLinkPropO			"APPLELINK"
#define VCAttachPropO			"ATTACH"
#define VCAttendeePropO			"ATTENDEE"
#define VCATTMailPropO			"ATTMAIL"
#define VCAudioContentPropO		"AUDIOCONTENT"
#define VCAVIPropO				"AVI"
#define VCBase64PropO			"BASE64"
#define VCBBSPropO				"BBS"
#define VCBirthDatePropO			"BDAY"
#define VCBMPPropO				"BMP"
#define VCBodyPropO				"BODY"
#define VCBusinessRolePropO		"ROLE"
#define VCCalPropO				"VCALENDAR"
#define VCCaptionPropO			"CAP"
#define VCCardPropO				"VCARD"
#define VCCarPropO				"CAR"
#define VCCategoriesPropO		"CATEGORIES"
#define VCCellularPropO			"CELL"
#define VCCGMPropO				"CGM"
#define VCCharSetPropO			"CS"
#define VCCIDPropO				"CID"
#define VCCISPropO				"CIS"
#define VCCityPropO				"L"
#define VCClassPropO				"CLASS"
#define VCCommentPropO			"NOTE"
#define VCCompletedPropO			"COMPLETED"
#define VCContentIDPropO			"CONTENT-ID"
#define VCCountryNamePropO		"C"
#define VCDAlarmPropO			"DALARM"
#define VCDataSizePropO			"DATASIZE"
#define VCDayLightPropO			"DAYLIGHT"
#define VCDCreatedPropO			"DCREATED"
#define VCDeliveryLabelPropO     "LABEL"
#define VCDescriptionPropO		"DESCRIPTION"
#define VCDIBPropO				"DIB"
#define VCDisplayStringPropO		"DISPLAYSTRING"
#define VCDomesticPropO			"DOM"
#define VCDTendPropO				"DTEND"
#define VCDTstartPropO			"DTSTART"
#define VCDuePropO				"DUE"
#define VCEmailAddressPropO		"EMAIL"
#define VCEncodingPropO			"ENCODING"
#define VCEndPropO				"END"
#define VCEventPropO				"VEVENT"
#define VCEWorldPropO			"EWORLD"
#define VCExNumPropO				"EXNUM"
#define VCExpDatePropO			"EXDATE"
#define VCExpectPropO			"EXPECT"
#define VCExtAddressPropO		"EXT ADD"
#define VCFamilyNamePropO		"F"
#define VCFaxPropO				"FAX"
#define VCFullNamePropO			"FN"
#define VCGeoPropO				"GEO"
#define VCGeoLocationPropO		"GEO"
#define VCGIFPropO				"GIF"
#define VCGivenNamePropO			"G"
#define VCGroupingPropO			"Grouping"
#define VCHomePropO				"HOME"
#define VCIBMMailPropO			"IBMMail"
#define VCInlinePropO			"INLINE"
#define VCInternationalPropO		"INTL"
#define VCInternetPropO			"INTERNET"
#define VCISDNPropO				"ISDN"
#define VCJPEGPropO				"JPEG"
#define VCLanguagePropO			"LANG"
#define VCLastModifiedPropO		"LAST-MODIFIED"
#define VCLastRevisedPropO		"REV"
#define VCLocationPropO			"LOCATION"
#define VCLogoPropO				"LOGO"
#define VCMailerPropO			"MAILER"
#define VCMAlarmPropO			"MALARM"
#define VCMCIMailPropO			"MCIMAIL"
#define VCMessagePropO			"MSG"
#define VCMETPropO				"MET"
#define VCModemPropO				"MODEM"
#define VCMPEG2PropO				"MPEG2"
#define VCMPEGPropO				"MPEG"
#define VCMSNPropO				"MSN"
#define VCNamePrefixesPropO		"NPRE"
#define VCNamePropO				"N"
#define VCNameSuffixesPropO		"NSUF"
#define VCNotePropO				"NOTE"
#define VCOrgNamePropO			"ORGNAME"
#define VCOrgPropO				"ORG"
#define VCOrgUnit2PropO			"OUN2"
#define VCOrgUnit3PropO			"OUN3"
#define VCOrgUnit4PropO			"OUN4"
#define VCOrgUnitPropO			"OUN"
#define VCPagerPropO				"PAGER"
#define VCPAlarmPropO			"PALARM"
#define VCParcelPropO			"PARCEL"
#define VCPartPropO				"PART"
#define VCPCMPropO				"PCM"
#define VCPDFPropO				"PDF"
#define VCPGPPropO				"PGP"
#define VCPhotoPropO				"PHOTO"
#define VCPICTPropO				"PICT"
#define VCPMBPropO				"PMB"
#define VCPostalBoxPropO			"BOX"
#define VCPostalCodePropO		"PC"
#define VCPostalPropO			"POSTAL"
#define VCPowerSharePropO		"POWERSHARE"
#define VCPreferredPropO			"PREF"
#define VCPriorityPropO			"PRIORITY"
#define VCProcedureNamePropO		"PROCEDURENAME"
#define VCProdIdPropO			"PRODID"
#define VCProdigyPropO			"PRODIGY"
#define VCPronunciationPropO		"SOUND"
#define VCPSPropO				"PS"
#define VCPublicKeyPropO			"KEY"
#define VCQPPropO				"QP"
#define VCQuickTimePropO			"QTIME"
#define VCQuotedPrintablePropO	"QUOTED-PRINTABLE"
#define VCRDatePropO				"RDATE"
#define VCRegionPropO			"R"
#define VCRelatedToPropO			"RELATED-TO"
#define VCRepeatCountPropO		"REPEATCOUNT"
#define VCResourcesPropO			"RESOURCES"
#define VCRNumPropO				"RNUM"
#define VCRolePropO				"ROLE"
#define VCRRulePropO				"RRULE"
#define VCRSVPPropO				"RSVP"
#define VCRunTimePropO			"RUNTIME"
#define VCSequencePropO			"SEQUENCE"
#define VCSnoozeTimePropO		"SNOOZETIME"
#define VCStartPropO				"START"
#define VCStatusPropO			"STATUS"
#define VCStreetAddressPropO		"STREET"
#define VCSubTypePropO			"SUBTYPE"
#define VCSummaryPropO			"SUMMARY"
#define VCTelephonePropO			"TEL"
#define VCTIFFPropO				"TIFF"
#define VCTimeZonePropO			"TZ"
#define VCTitlePropO				"TITLE"
#define VCTLXPropO				"TLX"
#define VCTodoPropO				"VTODO"
#define VCTranspPropO			"TRANSP"
#define VCUniqueStringPropO		"UID"
#define VCURLPropO				"URL"
#define VCURLValuePropO			"URLVAL"
#define VCValuePropO				"VALUE"
#define VCVersionPropO			"VERSION"
#define VCVideoPropO				"VIDEO"
#define VCVoicePropO				"VOICE"
#define VCWAVEPropO				"WAVE"
#define VCWMFPropO				"WMF"
#define VCWorkPropO				"WORK"
#define VCX400PropO				"X400"
#define VCX509PropO				"X509"
#define VCXRulePropO				"XRULE"

/* Extensions */

#define XPilotIdPropO                            "X-PILOTID"
#define XPilotStatusPropO                        "X-PILOTSTAT"
#define XPilotNoTimePropO                        "X-PILOT-NOTIME"


/* Added for multisync */
#define VCDTStampPropO				"DTSTAMP"
#define VCPctCompletePropO			"PERCENT-COMPLETE"
#define VCTzidPropO				"TZID"
#define VCFreqPropO				"FREQ"
#define VCUntilPropO				"UNTIL"
#define VCIntervalPropO				"INTERVAL"
#define VCByDayPropO				"BYDAY"
#define VCBySetPosPropO				"BYSETPOS"


/* VALARM */
#define VCAlarmPropO				"VALARM"
#define VCTriggerPropO				"TRIGGER"
#define VCRelatedPropO				"RELATED"
#define VCActionPropO				"ACTION"

/* Ximian Evolution extensions */
#define XEvoAlarmUidO				"X-EVOLUTION-ALARM-UID"
#define XEvoFileAsO				"X-EVOLUTION-FILE-AS"
#define XEvolutionSpouseO                       "X-EVOLUTION-SPOUSE"
#define XEvolutionAssistantO                    "X-EVOLUTION-ASSISTANT"
#define XEvolutionManagerO                      "X-EVOLUTION-MANAGER"
#define XEvolutionOfficeO                       "X-EVOLUTION-OFFICE"
#define XEvolutionAnniversaryO                  "X-EVOLUTION-ANNIVERSARY"

#define VCNicknameO                             "NICKNAME"
#define VCCalendarURIO                          "CALURI"
#define VCFBURLO                                "FBURL"

typedef struct VObjectO VObjectO;

typedef struct VObjectIteratorO {
    VObjectO* start;
    VObjectO* next;
    } VObjectIteratorO;

extern DLLEXPORT(VObjectO*) newVObjectO(const char *id);
extern DLLEXPORT(void) deleteVObjectO(VObjectO *p);
extern DLLEXPORT(char*) dupStrO(const char *s, unsigned int size);
extern DLLEXPORT(void) deleteStrO(const char *p);
extern DLLEXPORT(void) unUseStrO(const char *s);

extern DLLEXPORT(void) setVObjectNameO(VObjectO *o, const char* id);
extern DLLEXPORT(void) setVObjectStringZValueO(VObjectO *o, const char *s);
extern DLLEXPORT(void) setVObjectStringZValue_O(VObjectO *o, const char *s);
extern DLLEXPORT(void) setVObjectUStringZValueO(VObjectO *o, const wchar_t *s);
extern DLLEXPORT(void) setVObjectUStringZValue_O(VObjectO *o, const wchar_t *s);
extern DLLEXPORT(void) setVObjectIntegerValueO(VObjectO *o, unsigned int i);
extern DLLEXPORT(void) setVObjectLongValueO(VObjectO *o, unsigned long l);
extern DLLEXPORT(void) setVObjectAnyValueO(VObjectO *o, void *t);
extern DLLEXPORT(VObjectO*) setValueWithSizeO(VObjectO *prop, void *val, unsigned int size);
extern DLLEXPORT(VObjectO*) setValueWithSize_O(VObjectO *prop, void *val, unsigned int size);

extern DLLEXPORT(const char*) vObjectNameO(VObjectO *o);
extern DLLEXPORT(const char*) vObjectStringZValueO(VObjectO *o);
extern DLLEXPORT(const wchar_t*) vObjectUStringZValueO(VObjectO *o);
extern DLLEXPORT(unsigned int) vObjectIntegerValueO(VObjectO *o);
extern DLLEXPORT(unsigned long) vObjectLongValueO(VObjectO *o);
extern DLLEXPORT(void*) vObjectAnyValueO(VObjectO *o);
extern DLLEXPORT(VObjectO*) vObjectVObjectValueO(VObjectO *o);
extern DLLEXPORT(void) setVObjectNullValueO(VObjectO *o);
extern DLLEXPORT(void) setVObjectVObjectValueO(VObjectO *o, VObjectO *p);

extern DLLEXPORT(VObjectO*) addVObjectPropO(VObjectO *o, VObjectO *p);
extern DLLEXPORT(VObjectO*) addPropO(VObjectO *o, const char *id);
extern DLLEXPORT(VObjectO*) addProp_O(VObjectO *o, const char *id);
extern DLLEXPORT(VObjectO*) addPropValueO(VObjectO *o, const char *p, const char *v);
extern DLLEXPORT(VObjectO*) addPropSizedValue_O(VObjectO *o, const char *p, const char *v, unsigned int size);
extern DLLEXPORT(VObjectO*) addPropSizedValueO(VObjectO *o, const char *p, const char *v, unsigned int size);
extern DLLEXPORT(VObjectO*) addGroupO(VObjectO *o, const char *g);
extern DLLEXPORT(void) addListO(VObjectO **o, VObjectO *p);

extern DLLEXPORT(VObjectO*) isAPropertyOfO(VObjectO *o, const char *id);

extern DLLEXPORT(VObjectO*) nextVObjectInListO(VObjectO *o);
extern DLLEXPORT(void) initPropIteratorO(VObjectIteratorO *i, VObjectO *o);
extern DLLEXPORT(int) moreIterationO(VObjectIteratorO *i);
extern DLLEXPORT(VObjectO*) nextVObjectO(VObjectIteratorO *i);

extern DLLEXPORT(char*) writeMemVObjectO(char *s, int *len, VObjectO *o);
extern DLLEXPORT(char*) writeMemVObjectsO(char *s, int *len, VObjectO *list);

extern DLLEXPORT(const char*) lookupStrO(const char *s);
extern DLLEXPORT(void) cleanStrTblO();

extern DLLEXPORT(void) cleanVObjectO(VObjectO *o);
extern DLLEXPORT(void) cleanVObjectsO(VObjectO *list);

extern DLLEXPORT(const char*) lookupPropO(const char* str);
extern DLLEXPORT(const char*) lookupProp_O(const char* str);

extern DLLEXPORT(wchar_t*) fakeUnicodeO(const char *ps, int *bytes);
extern DLLEXPORT(int) uStrLenO(const wchar_t *u);
extern DLLEXPORT(char*) fakeCStringO(const wchar_t *u);

extern DLLEXPORT(void) printVObjectToFileO(char *fname,VObjectO *o);
extern DLLEXPORT(void) printVObjectsToFileO(char *fname,VObjectO *list);
extern DLLEXPORT(void) writeVObjectToFileO(char *fname, VObjectO *o);
extern DLLEXPORT(void) writeVObjectsToFileO(char *fname, VObjectO *list);

extern DLLEXPORT(int) vObjectValueTypeO(VObjectO *o);

/* return type of vObjectValueType: */
#define VCVT_NOVALUE	0
	/* if the VObject has no value associated with it. */
#define VCVT_STRINGZ	1
	/* if the VObject has value set by setVObjectStringZValue. */
#define VCVT_USTRINGZ	2
	/* if the VObject has value set by setVObjectUStringZValue. */
#define VCVT_UINT		3
	/* if the VObject has value set by setVObjectIntegerValue. */
#define VCVT_ULONG		4
	/* if the VObject has value set by setVObjectLongValue. */
#define VCVT_RAW		5
	/* if the VObject has value set by setVObjectAnyValue. */
#define VCVT_VOBJECT	6
	/* if the VObject has value set by setVObjectVObjectValue. */
#define VCVT_NULL	7

extern const char** fieldedPropO;

/* NOTE regarding printVObject and writeVObject

The functions below are not exported from the DLL because they
take a FILE* as a parameter, which cannot be passed across a DLL 
interface (at least that is my experience). Instead you can use
their companion functions which take file names or pointers
to memory. However, if you are linking this code into 
your build directly then you may find them a more convenient API
and you can go ahead and use them. If you try to use them with 
the DLL LIB you will get a link error.
*/
extern void printVObjectO(FILE *fp,VObjectO *o);
extern void writeVObjectO(FILE *fp, VObjectO *o);


#if defined(__CPLUSPLUS__) || defined(__cplusplus)
}
#endif

#endif /* __VOBJECT_H__ */


