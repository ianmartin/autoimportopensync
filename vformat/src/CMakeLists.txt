INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

ADD_LIBRARY( vformat SHARED vformat.c )
ADD_LIBRARY( xmlformat-common SHARED xmlformat-common.c )
ADD_LIBRARY( xmlformat-vcalendar SHARED xmlformat-vcalendar.c )


OPENSYNC_FORMAT_ADD( vcard vcard.c )
OPENSYNC_FORMAT_ADD( vevent vevent.c )
OPENSYNC_FORMAT_ADD( vnote vnote.c )
OPENSYNC_FORMAT_ADD( vtodo vtodo.c )


OPENSYNC_FORMAT_ADD( xmlformat-vcard xmlformat-vcard.c )
OPENSYNC_FORMAT_ADD( xmlformat-vevent xmlformat-vevent.c )
OPENSYNC_FORMAT_ADD( xmlformat-vnote xmlformat-vnote.c )
#OPENSYNC_FORMAT_ADD( xmlformat-vtodo xmlformat-vtodo.c )

TARGET_LINK_LIBRARIES( vcard ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vevent ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vnote ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vtodo ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

TARGET_LINK_LIBRARIES( vformat ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

TARGET_LINK_LIBRARIES( xmlformat-common ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} vformat )
TARGET_LINK_LIBRARIES( xmlformat-vcalendar ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} vformat xmlformat-common)

TARGET_LINK_LIBRARIES( xmlformat-vcard ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} xmlformat-common)
TARGET_LINK_LIBRARIES( xmlformat-vevent ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} xmlformat-vcalendar )
TARGET_LINK_LIBRARIES( xmlformat-vnote ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
#TARGET_LINK_LIBRARIES( xmlformat-vtodo ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

###### INSTALL ################### 
OPENSYNC_FORMAT_INSTALL( vcard )
OPENSYNC_FORMAT_INSTALL( vevent )
OPENSYNC_FORMAT_INSTALL( vnote )
OPENSYNC_FORMAT_INSTALL( vtodo )
OPENSYNC_FORMAT_INSTALL( xmlformat-vcard )
OPENSYNC_FORMAT_INSTALL( xmlformat-vevent )
OPENSYNC_FORMAT_INSTALL( xmlformat-vnote )
#OPENSYNC_FORMAT_INSTALL( xmlformat-vtodo )

INSTALL( TARGETS xmlformat-common xmlformat-vcalendar vformat DESTINATION ${OPENSYNC_FORMATSDIR} ) 

