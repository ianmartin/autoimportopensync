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
 * src: vobject.c
 * doc: VObjectO and APIs to construct vobject, APIs pretty print 
 * vobject, and convert a VObjectO into its textual representation.
 */

#include "vobject.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define USE_STRTBL

#define NAME_OF(o)				o->id
#define VALUE_TYPE(o)			o->valType
#define STRINGZ_VALUE_OF(o)		o->val.strs
#define USTRINGZ_VALUE_OF(o)	o->val.ustrs
#define INTEGER_VALUE_OF(o)		o->val.i
#define LONG_VALUE_OF(o)		o->val.l
#define ANY_VALUE_OF(o)			o->val.any
#define VOBJECT_VALUE_OF(o)		o->val.vobj

typedef union ValueItemO {
    const char *strs;
    const wchar_t *ustrs;
    unsigned int i;
    unsigned long l;
    void *any;
    VObjectO *vobj;
    } ValueItemO;

struct VObjectO {
    VObjectO *next;
    const char *id;
    VObjectO *prop;
    unsigned short valType;
    ValueItemO val;
    };

typedef struct StrItemO StrItemO;

struct StrItemO {
    StrItemO *next;
    const char *s;
    unsigned int refCnt;
    };

const char** fieldedPropO;


DLLEXPORT(VObjectO*) newVObject_O(const char *id);
DLLEXPORT(void) initVObjectIteratorO(VObjectIteratorO *i, VObjectO *o);
DLLEXPORT(void) cleanStrTblO();


/*----------------------------------------------------------------------
   The following functions involve with memory allocation:
	newVObject
	deleteVObject
	dupStr
	deleteStr
	newStrItem
	deleteStrItem
   ----------------------------------------------------------------------*/

DLLEXPORT(VObjectO*) newVObject_O(const char *id)
{
    VObjectO *p = (VObjectO*)malloc(sizeof(VObjectO));
    p->next = 0;
    p->id = id;
    p->prop = 0;
    VALUE_TYPE(p) = 0;
    ANY_VALUE_OF(p) = 0;
    return p;
}

DLLEXPORT(VObjectO*) newVObjectO(const char *id)
{
    return newVObject_O(lookupStrO(id));
}

DLLEXPORT(void) deleteVObjectO(VObjectO *p)
{
    unUseStrO(p->id);
    free(p);
}

DLLEXPORT(char*) dupStrO(const char *s, unsigned int size)
{
    char *t;
    if  (size == 0) {
	size = strlen(s);
	}
    t = (char*)malloc(size+1);
    if (t) {
	memcpy(t,s,size);
	t[size] = 0;
	return t;
	}
    else {
	return (char*)0;
	}
}

DLLEXPORT(void) deleteStrO(const char *p)
{
    if (p) free((void*)p);
}


#ifdef USE_STRTBL
static StrItemO* newStrItemO(const char *s, StrItemO *next)
{
    StrItemO *p = (StrItemO*)malloc(sizeof(StrItemO));
    p->next = next;
    p->s = s;
    p->refCnt = 1;
    return p;
}

static void deleteStrItemO(StrItemO *p)
{
    free((void*)p);
}
#endif

/*----------------------------------------------------------------------
  The following function provide accesses to VObject's value.
  ----------------------------------------------------------------------*/

DLLEXPORT(const char*) vObjectNameO(VObjectO *o)
{
    return NAME_OF(o);
}

DLLEXPORT(void) setVObjectNameO(VObjectO *o, const char* id)
{
    NAME_OF(o) = id;
}

DLLEXPORT(const char*) vObjectStringZValueO(VObjectO *o)
{
    return STRINGZ_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectStringZValueO(VObjectO *o, const char *s)
{
    STRINGZ_VALUE_OF(o) = dupStrO(s,0);
    VALUE_TYPE(o) = VCVT_STRINGZ;
}

DLLEXPORT(void) setVObjectStringZValue_O(VObjectO *o, const char *s)
{
    STRINGZ_VALUE_OF(o) = s;
    VALUE_TYPE(o) = VCVT_STRINGZ;
}

DLLEXPORT(const wchar_t*) vObjectUStringZValueO(VObjectO *o)
{
    return USTRINGZ_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectUStringZValueO(VObjectO *o, const wchar_t *s)
{
    USTRINGZ_VALUE_OF(o) = (wchar_t*) dupStrO((char*)s,(uStrLenO(s)+1)*2);
    VALUE_TYPE(o) = VCVT_USTRINGZ;
}

DLLEXPORT(void) setVObjectUStringZValue_O(VObjectO *o, const wchar_t *s)
{
    USTRINGZ_VALUE_OF(o) = s;
    VALUE_TYPE(o) = VCVT_USTRINGZ;
}

DLLEXPORT(unsigned int) vObjectIntegerValueO(VObjectO *o)
{
    return INTEGER_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectIntegerValueO(VObjectO *o, unsigned int i)
{
    INTEGER_VALUE_OF(o) = i;
    VALUE_TYPE(o) = VCVT_UINT;
}

DLLEXPORT(unsigned long) vObjectLongValueO(VObjectO *o)
{
    return LONG_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectLongValueO(VObjectO *o, unsigned long l)
{
    LONG_VALUE_OF(o) = l;
    VALUE_TYPE(o) = VCVT_ULONG;
}

DLLEXPORT(void*) vObjectAnyValueO(VObjectO *o)
{
    return ANY_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectAnyValueO(VObjectO *o, void *t)
{
    ANY_VALUE_OF(o) = t;
    VALUE_TYPE(o) = VCVT_RAW;
}

DLLEXPORT(VObjectO*) vObjectVObjectValueO(VObjectO *o)
{
    return VOBJECT_VALUE_OF(o);
}

DLLEXPORT(void) setVObjectNullValueO(VObjectO *o)
{
    VALUE_TYPE(o) = VCVT_NULL;
}

DLLEXPORT(void) setVObjectVObjectValueO(VObjectO *o, VObjectO *p)
{
    VOBJECT_VALUE_OF(o) = p;
    VALUE_TYPE(o) = VCVT_VOBJECT;
}

DLLEXPORT(int) vObjectValueTypeO(VObjectO *o)
{
    return VALUE_TYPE(o);
}


/*----------------------------------------------------------------------
  The following functions can be used to build VObject.
  ----------------------------------------------------------------------*/

DLLEXPORT(VObjectO*) addVObjectPropO(VObjectO *o, VObjectO *p)
{
    /* circular link list pointed to tail */
    /*
    o {next,id,prop,val}
                V
	pn {next,id,prop,val}
             V
	    ...
	p1 {next,id,prop,val}
             V
	     pn
    -->
    o {next,id,prop,val}
                V
	pn {next,id,prop,val}
             V
	p {next,id,prop,val}
	    ...
	p1 {next,id,prop,val}
             V
	     pn
    */

    VObjectO *tail = o->prop;
    if (tail) {
	p->next = tail->next;
	o->prop = tail->next = p;
	}
    else {
	o->prop = p->next = p;
	}
    return p;
}

DLLEXPORT(VObjectO*) addPropO(VObjectO *o, const char *id)
{
    return addVObjectPropO(o,newVObjectO(id));
}

DLLEXPORT(VObjectO*) addProp_O(VObjectO *o, const char *id)
{
    return addVObjectPropO(o,newVObject_O(id));
}

DLLEXPORT(void) addListO(VObjectO **o, VObjectO *p)
{
    p->next = 0;
    if (*o == 0) {
	*o = p;
	}
    else {
	VObjectO *t = *o;
	while (t->next) {
	   t = t->next;
	   }
	t->next = p;
	}
}

DLLEXPORT(VObjectO*) nextVObjectInListO(VObjectO *o)
{
    return o->next;
}

DLLEXPORT(VObjectO*) setValueWithSize_O(VObjectO *prop, void *val, unsigned int size)
{
    VObjectO *sizeProp;
    setVObjectAnyValueO(prop, val);
    sizeProp = addPropO(prop,VCDataSizePropO);
    setVObjectLongValueO(sizeProp, size);
    return prop;
}

DLLEXPORT(VObjectO*) setValueWithSizeO(VObjectO *prop, void *val, unsigned int size)
{
    void *p = dupStrO((const char *)val,size);
    return setValueWithSize_O(prop,p,p?size:0);
}

DLLEXPORT(void) initPropIteratorO(VObjectIteratorO *i, VObjectO *o)
{
    i->start = o->prop; 
    i->next = 0;
}

DLLEXPORT(void) initVObjectIteratorO(VObjectIteratorO *i, VObjectO *o)
{
    i->start = o->next; 
    i->next = 0;
}

DLLEXPORT(int) moreIterationO(VObjectIteratorO *i)
{ 
    return (i->start && (i->next==0 || i->next!=i->start));
}

DLLEXPORT(VObjectO*) nextVObjectO(VObjectIteratorO *i)
{
    if (i->start && i->next != i->start) {
	if (i->next == 0) {
	    i->next = i->start->next;
	    return i->next;
	    }
	else {
	    i->next = i->next->next;
	    return i->next;
	    }
	}
    else return (VObjectO*)0;
}

DLLEXPORT(VObjectO*) isAPropertyOfO(VObjectO *o, const char *id)
{
    VObjectIteratorO i;
    initPropIteratorO(&i,o);
    while (moreIterationO(&i)) {
	VObjectO *each = nextVObjectO(&i);
	if (!stricmp(id,each->id))
	    return each;
	}
    return (VObjectO*)0;
}

DLLEXPORT(VObjectO*) addGroupO(VObjectO *o, const char *g)
{
    /*
	a.b.c
	-->
	prop(c)
	    prop(VCGrouping=b)
		prop(VCGrouping=a)
     */
    char *dot = strrchr(g,'.');
    if (dot) {
	VObjectO *p, *t;
	char *gs, *n = dot+1;
	gs = dupStrO(g,0);	/* so we can write to it. */
	/* used to be
	* t = p = addProp_(o,lookupProp_(n));
	*/
	t = p = addProp_O(o,lookupPropO(n));
	dot = strrchr(gs,'.');
	*dot = 0;
	do {
	    dot = strrchr(gs,'.');
	    if (dot) {
		n = dot+1;
		*dot=0;
		}
	    else
		n = gs;
	    /* property(VCGroupingProp=n);
	     *	and the value may have VCGrouping property
	     */
	    t = addPropO(t,VCGroupingPropO);
	    setVObjectStringZValueO(t,lookupProp_O(n));
	    } while (n != gs);
	deleteStrO(gs);	
	return p;
	}
    else
	return addProp_O(o,lookupPropO(g));
}

DLLEXPORT(VObjectO*) addPropValueO(VObjectO *o, const char *p, const char *v)
{
    VObjectO *prop;
    prop = addPropO(o,p);
    setVObjectUStringZValue_O(prop, fakeUnicodeO(v,0));
    return prop;
}

DLLEXPORT(VObjectO*) addPropSizedValue_O(VObjectO *o, const char *p, const char *v,
	unsigned int size)
{
    VObjectO *prop;
    prop = addPropO(o,p);
    setValueWithSize_O(prop, (void*)v, size);
    return prop;
}

DLLEXPORT(VObjectO*) addPropSizedValueO(VObjectO *o, const char *p, const char *v,
	unsigned int size)
{
    return addPropSizedValue_O(o,p,dupStrO(v,size),size);
}



/*----------------------------------------------------------------------
  The following pretty print a VObject
  ----------------------------------------------------------------------*/

static void printVObject_O(FILE *fp, VObjectO *o, int level);

static void indentO(FILE *fp, int level)
{
    int i;
    for (i=0;i<level*4;i++) {
	fputc(' ', fp);
	}
}

static void printValueO(FILE *fp, VObjectO *o, int level)
{
    switch (VALUE_TYPE(o)) {
	case VCVT_USTRINGZ: {
	    char c;
            char *t,*s;
	    s = t = fakeCStringO(USTRINGZ_VALUE_OF(o));
	    fputc('"',fp);
	    while (c=*t,c) {
	        fputc(c,fp);
		if (c == '\n') indentO(fp,level+2);
		t++;
		}
	    fputc('"',fp);
	    deleteStrO(s);
	    break;
	    }
	case VCVT_STRINGZ: {
	    char c;
	    const char *s = STRINGZ_VALUE_OF(o);
	    fputc('"',fp);
	    while (c=*s,c) {
	        fputc(c,fp);
		if (c == '\n') indentO(fp,level+2);
		s++;
		}
	    fputc('"',fp);
	    break;
	    }
	case VCVT_UINT:
	    fprintf(fp,"%d", INTEGER_VALUE_OF(o)); break;
	case VCVT_ULONG:
	    fprintf(fp,"%ld", LONG_VALUE_OF(o)); break;
	case VCVT_RAW:
	    fprintf(fp,"[raw data]"); break;
	case VCVT_VOBJECT:
	    fprintf(fp,"[vobject]\n");
	    printVObject_O(fp,VOBJECT_VALUE_OF(o),level+1);
	    break;
	case 0:
	    fprintf(fp,"[none]"); break;
	default:
	    fprintf(fp,"[unknown]"); break;
	}
}

static void printNameValueO(FILE *fp,VObjectO *o, int level)
{
    indentO(fp,level);
    if (NAME_OF(o)) {
	fprintf(fp,"%s", NAME_OF(o));
	}
    if (VALUE_TYPE(o)) {
	fputc('=',fp);
	printValueO(fp,o, level);
	}
    fprintf(fp,"\n");
}

static void printVObject_O(FILE *fp, VObjectO *o, int level)
    {
    VObjectIteratorO t;
    if (o == 0) {
	fprintf(fp,"[NULL]\n");
	return;
	}
    printNameValueO(fp,o,level);
    initPropIteratorO(&t,o);
    while (moreIterationO(&t)) {
	VObjectO *eachProp = nextVObjectO(&t);
	printVObject_O(fp,eachProp,level+1);
	}
    }

void printVObjectO(FILE *fp,VObjectO *o)
{
    printVObject_O(fp,o,0);
}

DLLEXPORT(void) printVObjectToFileO(char *fname,VObjectO *o)
{
    FILE *fp = fopen(fname,"w");
    if (fp) {
	printVObjectO(fp,o);
	fclose(fp);
	}
}

DLLEXPORT(void) printVObjectsToFileO(char *fname,VObjectO *list)
{
    FILE *fp = fopen(fname,"w");
    if (fp) {
	while (list) {
	    printVObjectO(fp,list);
	    list = nextVObjectInListO(list);
	    }
	fclose(fp);
	}
}

DLLEXPORT(void) cleanVObjectO(VObjectO *o)
{
    if (o == 0) return;
    if (o->prop) {
	/* destroy time: cannot use the iterator here.
	   Have to break the cycle in the circular link
	   list and turns it into regular NULL-terminated
	   list -- since at some point of destruction,
	   the reference entry for the iterator to work
	   will not longer be valid.
	   */
	VObjectO *p;
	p = o->prop->next;
	o->prop->next = 0;
	do {
	   VObjectO *t = p->next;
	   cleanVObjectO(p);
	   p = t;
	   } while (p);
	}
    switch (VALUE_TYPE(o)) {
	case VCVT_USTRINGZ:
	case VCVT_STRINGZ:
	case VCVT_RAW:
		/* assume they are all allocated by malloc. */
	    free((char*)STRINGZ_VALUE_OF(o));
	    break;
	case VCVT_VOBJECT:
	    cleanVObjectO(VOBJECT_VALUE_OF(o));
	    break;
	}
    deleteVObjectO(o);
}

DLLEXPORT(void) cleanVObjectsO(VObjectO *list)
{
    while (list) {
	VObjectO *t = list;
	list = nextVObjectInListO(list);
	cleanVObjectO(t);
	}
}

/*----------------------------------------------------------------------
  The following is a String Table Facilities.
  ----------------------------------------------------------------------*/
#ifdef USE_STRTBL

#define STRTBLSIZE 255

static StrItemO *strTblO[STRTBLSIZE];

static unsigned int hashStrO(const char *s)
{
    unsigned int h = 0;
    int i;
    for (i=0;s[i];i++) {
	h += s[i]*i;
	}
    return h % STRTBLSIZE;
}

#endif

DLLEXPORT(const char*) lookupStrO(const char *s)
{
#ifdef USE_STRTBL
    StrItemO *t;
    unsigned int h = hashStrO(s);
    if ((t = strTblO[h]) != 0) {
	do {
	    if (stricmp(t->s,s) == 0) {
		t->refCnt++;
		return t->s;
		}
	    t = t->next;
	    } while (t);
	}
    s = dupStrO(s,0);
    strTblO[h] = newStrItemO(s,strTblO[h]);
    return s;
#else
  return dupStrO(s, 0);
#endif
}

DLLEXPORT(void) unUseStrO(const char *s)
{
#ifdef USE_STRTBL
    StrItemO *t, *p;
    unsigned int h = hashStrO(s);
    if ((t = strTblO[h]) != 0) {
	p = t;
	do {
	    if (stricmp(t->s,s) == 0) {
		t->refCnt--;
		if (t->refCnt == 0) {
		    if (t == strTblO[h]) {
			strTblO[h] = t->next;
			}
		    else {
			p->next = t->next;
			}
		    deleteStrO(t->s);
		    deleteStrItemO(t);
		    return;
		    }
		}
	    p = t;
	    t = t->next;
	    } while (t);
	}
#else
  deleteStrO (s);
#endif
}

DLLEXPORT(void) cleanStrTblO()
{
#ifdef USE_STRTBL
    int i;
    for (i=0; i<STRTBLSIZE;i++) {
	StrItemO *t = strTblO[i];
	while (t) {
	    StrItemO *p;
	    deleteStrO(t->s);
	    p = t;
	    t = t->next;
	    deleteStrItemO(p);
	    } while (t);
	strTblO[i] = 0;
	}
#endif
}


struct PreDefPropO {
    const char *name;
    const char *alias;
    const char** fields;
    unsigned int flags;
    };

/* flags in PreDefProp */
#define PD_BEGIN	0x1
#define PD_INTERNAL	0x2

static const char *adrFields0[] = {
    VCPostalBoxPropO,
    VCExtAddressPropO,
    VCStreetAddressPropO,
    VCCityPropO,
    VCRegionPropO,
    VCPostalCodePropO,
    VCCountryNamePropO,
    0
};

static const char *nameFields0[] = {
    VCFamilyNamePropO,
    VCGivenNamePropO,
    VCAdditionalNamesPropO,
    VCNamePrefixesPropO,
    VCNameSuffixesPropO,
    NULL
    };

static const char *orgFields0[] = {
    VCOrgNamePropO,
    VCOrgUnitPropO,
    VCOrgUnit2PropO,
    VCOrgUnit3PropO,
    VCOrgUnit4PropO,
    NULL
    };

static const char *AAlarmFields0[] = {
    VCRunTimePropO,
    VCSnoozeTimePropO,
    VCRepeatCountPropO,
    VCAudioContentPropO,
    0
    };

/* ExDate -- has unamed fields */
/* RDate -- has unamed fields */

static const char *DAlarmFields0[] = {
    VCRunTimePropO,
    VCSnoozeTimePropO,
    VCRepeatCountPropO,
    VCDisplayStringPropO,
    0
    };

static const char *MAlarmFields0[] = {
    VCRunTimePropO,
    VCSnoozeTimePropO,
    VCRepeatCountPropO,
    VCEmailAddressPropO,
    VCNotePropO,
    0
    };

static const char *PAlarmFields0[] = {
    VCRunTimePropO,
    VCSnoozeTimePropO,
    VCRepeatCountPropO,
    VCProcedureNamePropO,
    0
    };

static struct PreDefPropO propNamesO[] = {
    { VC7bitPropO, 0, 0, 0 },
    { VC8bitPropO, 0, 0, 0 },
    { VCAAlarmPropO, 0, AAlarmFields0, 0 },
    { VCAdditionalNamesPropO, 0, 0, 0 },
    { VCAdrPropO, 0, adrFields0, 0 },
    { VCAgentPropO, 0, 0, 0 },
    { VCAIFFPropO, 0, 0, 0 },
    { VCAOLPropO, 0, 0, 0 },
    { VCAppleLinkPropO, 0, 0, 0 },
    { VCAttachPropO, 0, 0, 0 },
    { VCAttendeePropO, 0, 0, 0 },
    { VCATTMailPropO, 0, 0, 0 },
    { VCAudioContentPropO, 0, 0, 0 },
    { VCAVIPropO, 0, 0, 0 },
    { VCBase64PropO, 0, 0, 0 },
    { VCBBSPropO, 0, 0, 0 },
    { VCBirthDatePropO, 0, 0, 0 },
    { VCBMPPropO, 0, 0, 0 },
    { VCBodyPropO, 0, 0, 0 },
    { VCBusinessRolePropO, 0, 0, 0 },
    { VCCalPropO, 0, 0, PD_BEGIN },
    { VCCaptionPropO, 0, 0, 0 },
    { VCCardPropO, 0, 0, PD_BEGIN },
    { VCCarPropO, 0, 0, 0 },
    { VCCategoriesPropO, 0, 0, 0 },
    { VCCellularPropO, 0, 0, 0 },
    { VCCGMPropO, 0, 0, 0 },
    { VCCharSetPropO, 0, 0, 0 },
    { VCCIDPropO, VCContentIDPropO, 0, 0 },
    { VCCISPropO, 0, 0, 0 },
    { VCCityPropO, 0, 0, 0 },
    { VCClassPropO, 0, 0, 0 },
    { VCCommentPropO, 0, 0, 0 },
    { VCCompletedPropO, 0, 0, 0 },
    { VCContentIDPropO, 0, 0, 0 },
    { VCCountryNamePropO, 0, 0, 0 },
    { VCDAlarmPropO, 0, DAlarmFields0, 0 },
    { VCDataSizePropO, 0, 0, PD_INTERNAL },
    { VCDayLightPropO, 0, 0, 0 },
    { VCDCreatedPropO, 0, 0, 0 },
    { VCDeliveryLabelPropO, 0, 0, 0 },
    { VCDescriptionPropO, 0, 0, 0 },
    { VCDIBPropO, 0, 0, 0 },
    { VCDisplayStringPropO, 0, 0, 0 },
    { VCDomesticPropO, 0, 0, 0 },
    { VCDTendPropO, 0, 0, 0 },
    { VCDTstartPropO, 0, 0, 0 },
    { VCDuePropO, 0, 0, 0 },
    { VCEmailAddressPropO, 0, 0, 0 },
    { VCEncodingPropO, 0, 0, 0 },
    { VCEndPropO, 0, 0, 0 },
    { VCEventPropO, 0, 0, PD_BEGIN },
    { VCEWorldPropO, 0, 0, 0 },
    { VCExNumPropO, 0, 0, 0 },
    { VCExpDatePropO, 0, 0, 0 },
    { VCExpectPropO, 0, 0, 0 },
    { VCExtAddressPropO, 0, 0, 0 },
    { VCFamilyNamePropO, 0, 0, 0 },
    { VCFaxPropO, 0, 0, 0 },
    { VCFullNamePropO, 0, 0, 0 },
    { VCGeoLocationPropO, 0, 0, 0 },
    { VCGeoPropO, 0, 0, 0 },
    { VCGIFPropO, 0, 0, 0 },
    { VCGivenNamePropO, 0, 0, 0 },
    { VCGroupingPropO, 0, 0, 0 },
    { VCHomePropO, 0, 0, 0 },
    { VCIBMMailPropO, 0, 0, 0 },
    { VCInlinePropO, 0, 0, 0 },
    { VCInternationalPropO, 0, 0, 0 },
    { VCInternetPropO, 0, 0, 0 },
    { VCISDNPropO, 0, 0, 0 },
    { VCJPEGPropO, 0, 0, 0 },
    { VCLanguagePropO, 0, 0, 0 },
    { VCLastModifiedPropO, 0, 0, 0 },
    { VCLastRevisedPropO, 0, 0, 0 },
    { VCLocationPropO, 0, 0, 0 },
    { VCLogoPropO, 0, 0, 0 },
    { VCMailerPropO, 0, 0, 0 },
    { VCMAlarmPropO, 0, MAlarmFields0, 0 },
    { VCMCIMailPropO, 0, 0, 0 },
    { VCMessagePropO, 0, 0, 0 },
    { VCMETPropO, 0, 0, 0 },
    { VCModemPropO, 0, 0, 0 },
    { VCMPEG2PropO, 0, 0, 0 },
    { VCMPEGPropO, 0, 0, 0 },
    { VCMSNPropO, 0, 0, 0 },
    { VCNamePrefixesPropO, 0, 0, 0 },
    { VCNamePropO, 0, nameFields0, 0 },
    { VCNameSuffixesPropO, 0, 0, 0 },
    { VCNotePropO, 0, 0, 0 },
    { VCOrgNamePropO, 0, 0, 0 },
    { VCOrgPropO, 0, orgFields0, 0 },
    { VCOrgUnit2PropO, 0, 0, 0 },
    { VCOrgUnit3PropO, 0, 0, 0 },
    { VCOrgUnit4PropO, 0, 0, 0 },
    { VCOrgUnitPropO, 0, 0, 0 },
    { VCPagerPropO, 0, 0, 0 },
    { VCPAlarmPropO, 0, PAlarmFields0, 0 },
    { VCParcelPropO, 0, 0, 0 },
    { VCPartPropO, 0, 0, 0 },
    { VCPCMPropO, 0, 0, 0 },
    { VCPDFPropO, 0, 0, 0 },
    { VCPGPPropO, 0, 0, 0 },
    { VCPhotoPropO, 0, 0, 0 },
    { VCPICTPropO, 0, 0, 0 },
    { VCPMBPropO, 0, 0, 0 },
    { VCPostalBoxPropO, 0, 0, 0 },
    { VCPostalCodePropO, 0, 0, 0 },
    { VCPostalPropO, 0, 0, 0 },
    { VCPowerSharePropO, 0, 0, 0 },
    { VCPreferredPropO, 0, 0, 0 },
    { VCPriorityPropO, 0, 0, 0 },
    { VCProcedureNamePropO, 0, 0, 0 },
    { VCProdIdPropO, 0, 0, 0 },
    { VCProdigyPropO, 0, 0, 0 },
    { VCPronunciationPropO, 0, 0, 0 },
    { VCPSPropO, 0, 0, 0 },
    { VCPublicKeyPropO, 0, 0, 0 },
    { VCQPPropO, VCQuotedPrintablePropO, 0, 0 },
    { VCQuickTimePropO, 0, 0, 0 },
    { VCQuotedPrintablePropO, 0, 0, 0 },
    { VCRDatePropO, 0, 0, 0 },
    { VCRegionPropO, 0, 0, 0 },
    { VCRelatedToPropO, 0, 0, 0 },
    { VCRepeatCountPropO, 0, 0, 0 },
    { VCResourcesPropO, 0, 0, 0 },
    { VCRNumPropO, 0, 0, 0 },
    { VCRolePropO, 0, 0, 0 },
    { VCRRulePropO, 0, 0, 0 },
    { VCRSVPPropO, 0, 0, 0 },
    { VCRunTimePropO, 0, 0, 0 },
    { VCSequencePropO, 0, 0, 0 },
    { VCSnoozeTimePropO, 0, 0, 0 },
    { VCStartPropO, 0, 0, 0 },
    { VCStatusPropO, 0, 0, 0 },
    { VCStreetAddressPropO, 0, 0, 0 },
    { VCSubTypePropO, 0, 0, 0 },
    { VCSummaryPropO, 0, 0, 0 },
    { VCTelephonePropO, 0, 0, 0 },
    { VCTIFFPropO, 0, 0, 0 },
    { VCTimeZonePropO, 0, 0, 0 },
    { VCTitlePropO, 0, 0, 0 },
    { VCTLXPropO, 0, 0, 0 },
    { VCTodoPropO, 0, 0, PD_BEGIN },
    { VCTranspPropO, 0, 0, 0 },
    { VCUniqueStringPropO, 0, 0, 0 },
    { VCURLPropO, 0, 0, 0 },
    { VCURLValuePropO, 0, 0, 0 },
    { VCValuePropO, 0, 0, 0 },
    { VCVersionPropO, 0, 0, 0 },
    { VCVideoPropO, 0, 0, 0 },
    { VCVoicePropO, 0, 0, 0 },
    { VCWAVEPropO, 0, 0, 0 },
    { VCWMFPropO, 0, 0, 0 },
    { VCWorkPropO, 0, 0, 0 },
    { VCX400PropO, 0, 0, 0 },
    { VCX509PropO, 0, 0, 0 },
    { VCXRulePropO, 0, 0, 0 },
    { VCDTStampPropO, 0, 0, 0 },
    { VCPctCompletePropO, 0, 0, 0 },
    { VCTzidPropO, 0, 0, 0 },
    { VCAlarmPropO, 0, 0, PD_BEGIN },
    { VCTriggerPropO, 0, 0, 0 },
    { VCRelatedPropO, 0, 0, 0 },
    { VCActionPropO, 0, 0, 0 },
    { XEvoAlarmUidO, 0, 0, 0 },
    { VCFreqPropO, 0, 0, 0 },
    { VCUntilPropO, 0, 0, 0 },
    { VCIntervalPropO, 0, 0, 0 },
    { VCByDayPropO, 0, 0, 0 },
    { VCBySetPosPropO, 0, 0, 0 },
    { XEvolutionSpouseO, 0, 0, 0 },
    { XEvolutionAssistantO, 0, 0, 0 },
    { XEvolutionManagerO, 0, 0, 0 },
    { XEvolutionOfficeO, 0, 0, 0 },
    { XEvolutionAnniversaryO, 0, 0, 0 },
    { VCNicknameO, 0, 0, 0 },
    { VCCalendarURIO, 0, 0, 0 },
    { VCFBURLO, 0, 0, 0 },
    { 0,0,0,0 }
    };


static struct PreDefPropO* lookupPropInfoO(const char* str)
{
    /* brute force for now, could use a hash table here. */
    int i;
	
    for (i = 0; propNamesO[i].name; i++)
	if (stricmp(str, propNamesO[i].name) == 0) {
	    return &propNamesO[i];
	    }
    
    return 0;
}


DLLEXPORT(const char*) lookupProp_O(const char* str)
{
    int i;
	
    for (i = 0; propNamesO[i].name; i++)
	if (stricmp(str, propNamesO[i].name) == 0) {
	    const char* s;
	    s = propNamesO[i].alias?propNamesO[i].alias:propNamesO[i].name;
	    return lookupStrO(s);
	    }
    return lookupStrO(str);
}


DLLEXPORT(const char*) lookupPropO(const char* str)
{
    int i;
	
    for (i = 0; propNamesO[i].name; i++)
	if (stricmp(str, propNamesO[i].name) == 0) {
	    const char *s;
	    fieldedPropO = propNamesO[i].fields;
	    s = propNamesO[i].alias?propNamesO[i].alias:propNamesO[i].name;
	    return lookupStrO(s);
	    }
    fieldedPropO = 0;
    return lookupStrO(str);
}


/*----------------------------------------------------------------------
  APIs to Output text form.
  ----------------------------------------------------------------------*/
#define OFILE_REALLOC_SIZE 256
typedef struct OFileO {
    FILE *fp;
    char *s;
    int len;
    int limit;
    int alloc:1;
    int fail:1;
    } OFileO;

static void appendcOFile_O(OFileO *fp, char c)
{
    if (fp->fail) return;
    if (fp->fp) {
	fputc(c,fp->fp);
	}
    else {
stuff:
	if (fp->len+1 < fp->limit) {
	    fp->s[fp->len] = c;
	    fp->len++;
	    return;
	    }
	else if (fp->alloc) {
	    fp->limit = fp->limit + OFILE_REALLOC_SIZE;
	    fp->s = realloc(fp->s,fp->limit);
	    if (fp->s) goto stuff;
	    }
	if (fp->alloc)
	    free(fp->s);
	fp->s = 0;
	fp->fail = 1;
	}
}

static void appendcOFileO(OFileO *fp, char c)
{
    if (c == '\n') {
	/* write out as <CR><LF> */
	appendcOFile_O(fp,0xd);
	appendcOFile_O(fp,0xa);
	}
    else
	appendcOFile_O(fp,c);
}

static void appendsOFileO(OFileO *fp, const char *s)
{
    int i, slen;
    slen  = strlen(s);
    for (i=0; i<slen; i++) {
	appendcOFileO(fp,s[i]);
	}
}


static void initOFileO(OFileO *fp, FILE *ofp)
{
    fp->fp = ofp;
    fp->s = 0;
    fp->len = 0;
    fp->limit = 0;
    fp->alloc = 0;
    fp->fail = 0;
}

static void initMemOFileO(OFileO *fp, char *s, int len)
{
    fp->fp = 0;
    fp->s = s;
    fp->len = 0;
    fp->limit = s?len:0;
    fp->alloc = s?0:1;
    fp->fail = 0;
}


static int writeBase64O(OFileO *fp, unsigned char *s, long len)
{
    long cur = 0;
    int i, numQuads = 0;
    unsigned long trip;
    unsigned char b;
    char quad[5];
#define MAXQUADS 16

    quad[4] = 0;

    while (cur < len) {
	    /* collect the triplet of bytes into 'trip' */
	trip = 0;
	for (i = 0; i < 3; i++) {
	    b = (cur < len) ? *(s + cur) : 0;
	    cur++;
	    trip = trip << 8 | b;
	    }
	/* fill in 'quad' with the appropriate four characters */
	for (i = 3; i >= 0; i--) {
	    b = (unsigned char)(trip & 0x3F);
	    trip = trip >> 6;
	    if ((3 - i) < (cur - len))
		    quad[i] = '='; /* pad char */
	    else if (b < 26) quad[i] = (char)b + 'A';
	    else if (b < 52) quad[i] = (char)(b - 26) + 'a';
	    else if (b < 62) quad[i] = (char)(b - 52) + '0';
	    else if (b == 62) quad[i] = '+';
	    else quad[i] = '/';
	    }
	/* now output 'quad' with appropriate whitespace and line ending */
	appendsOFileO(fp, (numQuads == 0 ? "    " : ""));
	appendsOFileO(fp, quad);
	appendsOFileO(fp, ((cur >= len)?"\n" :(numQuads==MAXQUADS-1?"\n" : "")));
	numQuads = (numQuads + 1) % MAXQUADS;
	}
    appendcOFileO(fp,'\n');

    return 1;
}

static void writeStringO(OFileO *fp, const char *s)
{
    appendsOFileO(fp,s);
}

static void writeQPStringO(OFileO *fp, const char *s)
{
    char buf[4];
    int count=0;
    const char *p = s;

    while (*p) {
        /* break up lines biggger than 75 chars */
        if(count >=74){
		count=0;
		appendsOFileO(fp,"=\n");
	}
	
	/* escape any non ASCII characters and '=' as per rfc1521 */
	if (*p<= 0x1f || *p >=0x7f || *p == '=' ) {
		sprintf(buf,"=%02X",(unsigned char)*p);
		appendsOFileO(fp,buf); 
		count+=3; 
	} else {
		appendcOFileO(fp,*p);	
		count++; 
	}
	p++;
    }
}



static void writeVObject_O(OFileO *fp, VObjectO *o);

static void writeValueO(OFileO *fp, VObjectO *o, unsigned long size,int quote)
{
    if (o == 0) return;
    switch (VALUE_TYPE(o)) {
	case VCVT_USTRINGZ: {
	    char *s = fakeCStringO(USTRINGZ_VALUE_OF(o));
	    if(quote) writeQPStringO(fp, s);
	    else writeStringO(fp,s);
	    deleteStrO(s);
	    break;
	    }
	case VCVT_STRINGZ: {
	    if(quote) writeQPStringO(fp, STRINGZ_VALUE_OF(o));
	    else writeStringO(fp,STRINGZ_VALUE_OF(o));
	    break;
	    }
	case VCVT_UINT: {
	    char buf[16];
	    sprintf(buf,"%u", INTEGER_VALUE_OF(o));
	    appendsOFileO(fp,buf);
	    break;
	    }
	case VCVT_ULONG: {
	    char buf[16];
	    sprintf(buf,"%lu", LONG_VALUE_OF(o));
	    appendsOFileO(fp,buf);
	    break;
	    }
	case VCVT_RAW: {
	    appendcOFileO(fp,'\n');
	    writeBase64O(fp,(unsigned char*)(ANY_VALUE_OF(o)),size);
	    break;
	    }
	case VCVT_VOBJECT:
	    appendcOFileO(fp,'\n');
	    writeVObject_O(fp,VOBJECT_VALUE_OF(o));
	    break;
	}
}

static void writeAttrValueO(OFileO *fp, VObjectO *o)
{
    if (NAME_OF(o)) {
	struct PreDefPropO *pi;
	pi = lookupPropInfoO(NAME_OF(o));
	if (pi && ((pi->flags & PD_INTERNAL) != 0)) return;
	appendcOFileO(fp,';');
	appendsOFileO(fp,NAME_OF(o));
	}
    else
	appendcOFileO(fp,';');
    if (VALUE_TYPE(o)) {
	appendcOFileO(fp,'=');
	writeValueO(fp,o,0,0);
	}
}

static void writeGroupO(OFileO *fp, VObjectO *o)
{
    char buf1[256];
    char buf2[256];
    strcpy(buf1,NAME_OF(o));
    while ((o=isAPropertyOfO(o,VCGroupingPropO)) != 0) {
	strcpy(buf2,STRINGZ_VALUE_OF(o));
	strcat(buf2,".");
	strcat(buf2,buf1);
	strcpy(buf1,buf2);
	}
    appendsOFileO(fp,buf1);
}

static int inListO(const char **list, const char *s)
{
    if (list == 0) return 0;
    while (*list) {
	if (stricmp(*list,s) == 0) return 1;
	list++;
	}
    return 0;
}

static void writePropO(OFileO *fp, VObjectO *o)
{
    int isQuoted=0;
    if (NAME_OF(o)) {
	struct PreDefPropO *pi;
	VObjectIteratorO t;
	const char **fields_ = 0;
	pi = lookupPropInfoO(NAME_OF(o));
	if (pi && ((pi->flags & PD_BEGIN) != 0)) {
	    writeVObject_O(fp,o);
	    return;
	    }
	if (isAPropertyOfO(o,VCGroupingPropO))
	    writeGroupO(fp,o);
	else
	    appendsOFileO(fp,NAME_OF(o));
	if (pi) fields_ = pi->fields;
	initPropIteratorO(&t,o);
	while (moreIterationO(&t)) {
	    const char *s;
	    VObjectO *eachProp = nextVObjectO(&t);
	    s = NAME_OF(eachProp);
	    if (stricmp(VCGroupingPropO,s) && !inListO(fields_,s))
		writeAttrValueO(fp,eachProp);
	    if (stricmp(VCQPPropO,s)==0 || stricmp(VCQuotedPrintablePropO,s)==0 || (stricmp(VCEncodingPropO,s)==0 && (stricmp(VCQPPropO, fakeCStringO(vObjectUStringZValueO(eachProp))) == 0 || stricmp(VCQuotedPrintablePropO, fakeCStringO(vObjectUStringZValueO(eachProp))) == 0)))
		 isQuoted=1;
	    }
	if (fields_) {
	    int i = 0, n = 0;
	    const char** fields = fields_;
	    /* output prop as fields */
	    appendcOFileO(fp,':');
	    while (*fields) {
		VObjectO *t = isAPropertyOfO(o,*fields);
		i++;
		if (t) n = i;
		fields++;
		}
	    fields = fields_;
	    for (i=0;i<n;i++) {
		writeValueO(fp,isAPropertyOfO(o,*fields),0,isQuoted);
		fields++;
		if (i<(n-1)) appendcOFileO(fp,';');
		}
	    }
	}
	
    if (VALUE_TYPE(o)) {
	unsigned long size = 0;
        VObjectO *p = isAPropertyOfO(o,VCDataSizePropO);
	if (p) size = LONG_VALUE_OF(p);
	appendcOFileO(fp,':');
	writeValueO(fp,o,size,isQuoted);
	}

    appendcOFileO(fp,'\n');
}

static void writeVObject_O(OFileO *fp, VObjectO *o)
{
    if (NAME_OF(o)) {
	struct PreDefPropO *pi;
	pi = lookupPropInfoO(NAME_OF(o));

	if (pi && ((pi->flags & PD_BEGIN) != 0)) {
	    VObjectIteratorO t;
	    const char *begin = NAME_OF(o);
	    appendsOFileO(fp,"BEGIN:");
	    appendsOFileO(fp,begin);
	    appendcOFileO(fp,'\n');
	    initPropIteratorO(&t,o);
	    while (moreIterationO(&t)) {
		VObjectO *eachProp = nextVObjectO(&t);
		writePropO(fp, eachProp);
		}
	    appendsOFileO(fp,"END:");
	    appendsOFileO(fp,begin);
	    appendsOFileO(fp,"\n\n");
	    }
	}
}

void writeVObjectO(FILE *fp, VObjectO *o)
{
    OFileO ofp;
    initOFileO(&ofp,fp);
    writeVObject_O(&ofp,o);
}

DLLEXPORT(void) writeVObjectToFileO(char *fname, VObjectO *o)
{
    FILE *fp = fopen(fname,"w");
    if (fp) {
	writeVObjectO(fp,o);
	fclose(fp);
	}
}

DLLEXPORT(void) writeVObjectsToFileO(char *fname, VObjectO *list)
{
    FILE *fp = fopen(fname,"w");
    if (fp) {
	while (list) {
	    writeVObjectO(fp,list);
	    list = nextVObjectInListO(list);
	    }
	fclose(fp);
	}
}

DLLEXPORT(char*) writeMemVObjectO(char *s, int *len, VObjectO *o)
{
    OFileO ofp;
    initMemOFileO(&ofp,s,len?*len:0);
    writeVObject_O(&ofp,o);
    if (len) *len = ofp.len;
    appendcOFileO(&ofp,0);
    return ofp.s;
}

DLLEXPORT(char*) writeMemVObjectsO(char *s, int *len, VObjectO *list)
{
    OFileO ofp;
    initMemOFileO(&ofp,s,len?*len:0);
    while (list) {
	writeVObject_O(&ofp,list);
	list = nextVObjectInListO(list);
	}
    if (len) *len = ofp.len;
    appendcOFileO(&ofp,0);
    return ofp.s;
}

/*----------------------------------------------------------------------
  APIs to do fake Unicode stuff.
  ----------------------------------------------------------------------*/
DLLEXPORT(wchar_t*) fakeUnicodeO(const char *ps, int *bytes)
{
    wchar_t *r, *pw;
    int len = strlen(ps)+1;

    pw = r = (wchar_t*)malloc(sizeof(wchar_t)*len);
    if (bytes)
	*bytes = len * sizeof(wchar_t);

    while (*ps) { 
	if (*ps == '\n')
	    *pw = (wchar_t)0x2028;
	else if (*ps == '\r')
	    *pw = (wchar_t)0x2029;
	else
	    *pw = (wchar_t)(unsigned char)*ps;
	ps++; pw++;
	}				 
    *pw = (wchar_t)0;
	
    return r;
}

DLLEXPORT(int) uStrLenO(const wchar_t *u)
{
    int i = 0;
    
    if(!u)
      return 0;
      
    while (*u != (wchar_t)0) { u++; i++; }
    return i;
}

DLLEXPORT(char*) fakeCStringO(const wchar_t *u)
{
    char *s, *t;
    int len = 0;
    
    if (!u)
	return strdup("");
        
    len = uStrLenO(u) + 1;
    
    t = s = (char*)malloc(len);

    while (*u) {
	if (*u == (wchar_t)0x2028)
	    *t = '\n';
	else if (*u == (wchar_t)0x2029)
	    *t = '\r';
	else
	    *t = (char)*u;
	u++; t++;
	}
    *t = 0;
    return s;
}

/* end of source file vobject.c */
