INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ${OPENSYNC_INCLUDE_DIR} ${KDE3_INCLUDE_DIR} ${KDEPIM3_INCLUDE_DIR} ${QT_INCLUDE_DIR} )

# kdepim-sync sources
SET( kdepim_sync_LIB_SRCS
datasource.cpp
kaddrbook.cpp
kcal.cpp
kdepim_impl.cpp
kdepim_sync.cpp
knotes.cpp
)

ADD_DEFINITIONS( -DKDEPIM_LIBDIR="\\\"${OPENSYNC_PLUGINDIR}\\\"" )

KDE3_ADD_DCOP_STUBS( kdepim_sync_LIB_SRCS KNotesIface.h )

ADD_LIBRARY( kdepim-sync SHARED ${kdepim_sync_LIB_SRCS} )

TARGET_LINK_LIBRARIES( kdepim-sync ${OPENYNC_LIBRARIES} ${KDE3_LIBRARIES} ${KDEPIM3_KCAL_LIBRARIES} ${KDEPIM3_KABC_LIBRARIES} ${QT_LIBRARIES} )

# Install plugins	
INSTALL( TARGETS kdepim-sync DESTINATION ${OPENSYNC_PLUGINDIR} )

# Install config template	
INSTALL( FILES kdepim-sync DESTINATION ${OPENSYNC_CONFIGDIR} )

