INCLUDE_DIRECTORIES( ${GLIB2_INCLUDE_DIR} )

SET( osyncdump_SRCS
osyncdump.c
)

ADD_EXECUTABLE( osyncdump ${osyncdump_SRCS} )
TARGET_LINK_LIBRARIES( osyncdump opensync )



SET( osyncbinary_SRCS
osyncbinary.c		
)

ADD_EXECUTABLE( osyncbinary ${osyncbinary_SRCS} )
TARGET_LINK_LIBRARIES( osyncbinary opensync )

###### INSTALL ################### 

INSTALL( TARGETS osyncdump osyncbinary DESTINATION ${BIN_INSTALL_DIR} ) 

