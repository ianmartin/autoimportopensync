INCLUDE_DIRECTORIES( ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

ADD_LIBRARY( vcard SHARED vcard.c )
ADD_LIBRARY( vevent SHARED vevent.c )
ADD_LIBRARY( vnote SHARED vnote.c )
ADD_LIBRARY( vtodo SHARED vtodo.c )

ADD_LIBRARY( vformat SHARED vformat.c )

ADD_LIBRARY( xmlformat-common SHARED xmlformat-common.c )
ADD_LIBRARY( xmlformat-vcalendar SHARED xmlformat-vcalendar.c )

ADD_LIBRARY( xmlformat-vcard SHARED xmlformat-vcard.c )
ADD_LIBRARY( xmlformat-vevent SHARED xmlformat-vevent.c )
ADD_LIBRARY( xmlformat-vnote SHARED xmlformat-vnote.c )
#ADD_LIBRARY( xmlformat-vtodo SHARED xmlformat-vtodo.c )

TARGET_LINK_LIBRARIES( vcard ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vevent ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vnote ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( vtodo ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

TARGET_LINK_LIBRARIES( vformat ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

TARGET_LINK_LIBRARIES( xmlformat-common ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( xmlformat-vcalendar ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

TARGET_LINK_LIBRARIES( xmlformat-vcard ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( xmlformat-vevent ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
TARGET_LINK_LIBRARIES( xmlformat-vnote ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )
#TARGET_LINK_LIBRARIES( xmlformat-vtodo ${OPENSYNC_LIBRARY} ${GLIB2_LIBRARIES} )

###### INSTALL ################### 
INSTALL( TARGETS 
vcard 
vevent 
vnote 
vtodo 
vformat
xmlformat-common
xmlformat-vcalendar
xmlformat-vcard
xmlformat-vevent
xmlformat-vnote
#xmlformat-vtodo
DESTINATION ${OPENSYNC_FORMATSDIR} ) 

