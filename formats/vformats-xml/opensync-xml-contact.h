#ifndef HAVE_OPENSYNC_XML_CONTACT_H
#define HAVE_OPENSYNC_XML_CONTACT_H

/*
 * Use the constants in this header if you are going to convert
 * between "xml-contact" and another format. 
 *
 * See xml-vcard.c for an example.
 */


/* What this is */
#define XML_CONTACT_ROOT      "contact"

/* Parameters */
#define XML_CONTACT_TYPE      "Type"

/* Values for XML_CONTACT_TYPE */
#define XML_CONTACT_BULLETIN_BOARD  "BBS"
#define XML_CONTACT_CAR             "Car"
#define XML_CONTACT_CELLULAR        "Cell"
#define XML_CONTACT_DOMESTIC        "Domestic"
#define XML_CONTACT_FAX             "Fax"
#define XML_CONTACT_HOME            "Home"
#define XML_CONTACT_INTERNATIONAL   "International"
#define XML_CONTACT_INTERNET        "Internet"
#define XML_CONTACT_ISDN            "ISDN"
#define XML_CONTACT_MESSAGE         "Msg"
#define XML_CONTACT_MODEM           "Modem"
#define XML_CONTACT_PAGER           "Pager"
#define XML_CONTACT_PARCEL          "Parcel"
#define XML_CONTACT_PGP             "PGP"
#define XML_CONTACT_POSTAL          "Postal"
#define XML_CONTACT_PREF            "Pref"
#define XML_CONTACT_VIDEO           "Video"
#define XML_CONTACT_VOICE           "Voice"
#define XML_CONTACT_WORK            "Work"
#define XML_CONTACT_X509            "X509"

/* Lots of properties */
#define XML_CONTACT_ADDITIONAL        "Additional"
#define XML_CONTACT_ADDRESS           "Address"
#define XML_CONTACT_ADDRESS_LABEL     "AddressLabel"
#define XML_CONTACT_BIRTHDAY          "Birthday"
#define XML_CONTACT_CATEGORIES        "Categories"
#define XML_CONTACT_CITY              "City"
#define XML_CONTACT_CLASS             "Class"
#define XML_CONTACT_COUNTRY           "Country"
#define XML_CONTACT_EMAIL             "EMail"
#define XML_CONTACT_EXTENDED_ADDRESS  "ExtendedAddress"
#define XML_CONTACT_FILE_AS           "FileAs"
#define XML_CONTACT_FIRST_NAME        "FirstName"
#define XML_CONTACT_FULL_NAME         "FullName"
#define XML_CONTACT_KEY               "Key"
#define XML_CONTACT_LAST_NAME         "LastName"
#define XML_CONTACT_LOCATION          "Location"
#define XML_CONTACT_LOGO              "Logo"
#define XML_CONTACT_MAILER            "Mailer"
#define XML_CONTACT_NAME              "Name"
#define XML_CONTACT_NICKNAME          "Nickname"
#define XML_CONTACT_NOTE              "Note"
#define XML_CONTACT_ORGANIZATION      "Organization"
#define XML_CONTACT_PHOTO             "Photo"
#define XML_CONTACT_POSTAL_BOX        "PostalBox"
#define XML_CONTACT_POSTAL_CODE       "PostalCode"
#define XML_CONTACT_PREFIX            "Prefix"
#define XML_CONTACT_REGION            "Region"
#define XML_CONTACT_REVISION          "Revision"
#define XML_CONTACT_ROLE              "Role"
#define XML_CONTACT_SOUND             "Sound"
#define XML_CONTACT_STREET            "Street"
#define XML_CONTACT_SUFFIX            "Suffix"
#define XML_CONTACT_TELEPHONE         "Telephone"
#define XML_CONTACT_TIMEZONE          "Timezone"
#define XML_CONTACT_TITLE             "Title"
#define XML_CONTACT_UID               "Uid"
#define XML_CONTACT_UNIT              "Unit"
#define XML_CONTACT_URL               "Url"
#define XML_CONTACT_WANTS_HTML        "WantsHtml"

#define XML_CONTACT_CONTENT           "Content"

/* Unknown parameter */
#define XML_CONTACT_UNKNOWN_PARAM       "UnknownParam"
#define XML_CONTACT_PARAMETER_NAME      "ParamName"

/* Unknown node */
#define XML_CONTACT_UNKNOWN_NODE        "UnknownNode"
#define XML_CONTACT_NODE_NAME           "NodeName"

#endif
