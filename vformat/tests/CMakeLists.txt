INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CHECK_INCLUDE_DIR} ${OPENSYNC_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} )

########### support #################	
ADD_LIBRARY( support SHARED support.c )
TARGET_LINK_LIBRARIES( support opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} )

ADD_LIBRARY( conversion SHARED conversion.c )
TARGET_LINK_LIBRARIES( conversion opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} )

############ unit tests ##########################	

ADD_EXECUTABLE( vcard check_vcard.c )
ADD_EXECUTABLE( vevent check_vevent.c )
ADD_EXECUTABLE( vnote check_vnote.c )
ADD_EXECUTABLE( vtodo check_vtodo.c )

TARGET_LINK_LIBRARIES( vcard opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} support conversion )
TARGET_LINK_LIBRARIES( vevent opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} support conversion )
TARGET_LINK_LIBRARIES( vnote opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} support conversion )
TARGET_LINK_LIBRARIES( vtodo opensync ${CHECK_LIBRARIES} ${OPENSYNC_LIBRARIES} ${GLIB2_LIBRARIES} support conversion )

