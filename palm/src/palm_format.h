#ifndef VCARD_H
#define VCARD_H

char *tm2vcaldatetime(struct tm time);
char *tm2vcaldate(struct tm time);
struct tm vcaltime2tm(char *vcaltime);
char *isAAttributeOfO(char *rrule, char *pair);
void VObjectOErrorHander(char *errstr);
#endif /* VCARD_H */
