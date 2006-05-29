#ifndef SYNC_VTYPE_H
#define SYNC_VTYPE_H
#include <config.h>
#include <glib.h>

typedef enum {
  VOPTION_CONVERTUTC = 0x0001, // Convert UTC times to local time
  VOPTION_ADDUTF8CHARSET = 0x0002, // Add CHARSET=UTF-8 when unspecified
  VOPTION_FIXDSTTOCLIENT = 0x0004, // Fix a specific DST problem
  VOPTION_FIXDSTFROMCLIENT = 0x0008, // Fix a specific DST problem
  VOPTION_FIXCHARSET = 0x0010, // Convert from specified charset when unspec
  VOPTION_FIXTELOTHER = 0x0020, // Convert TEL: to TEL;VOICE: for Evolution
  VOPTION_CALENDAR2TO1 = 0x0040, // Convert vCALENDAR 2.0 to 1.0
  VOPTION_CALENDAR1TO2 = 0x0080, // Convert vCALENDAR 1.0 to 2.0
  VOPTION_REMOVEALARM = 0x0100, // Remove alarms
  VOPTION_REMOVEPHOTO = 0x0200, // Remove photo from vCARD
  VOPTION_REMOVEUTC = 0x0400, // Interpret UTC as local time
  VOPTION_CONVERTALLDAYEVENT = 0x0800 // Convert to vCAL 1.0 all day event (for some devices)
} sync_voption;

typedef enum {
  SYNC_COMPARE_DIFFERENT,
  SYNC_COMPARE_EQUAL,
  SYNC_COMPARE_SIMILAR
} sync_compare_result;

char* sync_vtype_convert(char *card, sync_voption opts, char *charset);
char *sync_vtype_encode_qp(char* in);
char* sync_vtype_decode_qp(char *in);
char* sync_timet_to_dt(time_t t);
char* sync_timet_to_dt_utc(time_t t);
time_t sync_dt_to_timet(char *str);
time_t sync_dur_to_timet(char *str);
char* sync_timet_to_dur(time_t dur);
char* sync_get_key_data(char *card, char *key);
char* sync_vtype_vcal1_to_vcal2(char* in);
char* sync_vtype_vcal2_to_vcal1(char* value);
char* sync_vtype_vcal2_list_to_vcal1(char *in);
char* sync_vtype_vcal1_list_to_vcal2(char **bits);

gboolean sync_compare_key_data(char *obj1, char *obj2, char *key);
gboolean sync_compare_key_times(char *obj1, char *obj2, char *key);

#endif
