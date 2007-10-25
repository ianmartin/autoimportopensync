INCLUDE_DIRECTORIES( ${GLIB2_INCLUDE_DIRS} ${LIBXML_INCLUDE_DIR} )

SET( file_SRCS
file.c		
)

ADD_LIBRARY( file SHARED ${file_SRCS} )
TARGET_LINK_LIBRARIES( file ${OPENSYNC_LIBRARIES} )

SET( plain_SRCS
plain.c
)

ADD_LIBRARY( plain SHARED ${plain_SRCS} )
TARGET_LINK_LIBRARIES( plain ${OPENSYNC_LIBRARIES} )

SET( xmlformat_SRCS
xmlformat.c
)

ADD_LIBRARY( xmlformat SHARED ${xmlformat_SRCS} )
TARGET_LINK_LIBRARIES( xmlformat ${OPENSYNC_LIBRARIES} )

SET( xmlformat_doc_SRCS
xmlformat-doc.c
)
ADD_LIBRARY( xmlformat-doc SHARED ${xmlformat_doc_SRCS} )
TARGET_LINK_LIBRARIES( xmlformat-doc ${OPENSYNC_LIBRARIES} )

###### INSTALL ##############	

INSTALL( TARGETS file plain xmlformat xmlformat-doc DESTINATION ${OPENSYNC_FORMATSDIR} )

INSTALL( FILES file.h xmlformat.h DESTINATION ${OPENSYNC_INCLUDE_DIR}/opensync )
