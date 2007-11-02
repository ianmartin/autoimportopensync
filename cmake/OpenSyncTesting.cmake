ENABLE_TESTING()
INCLUDE( CTest )

MACRO( OPENSYNC_ADD_TEST _testName _testSource ) 

  ADD_EXECUTABLE( ${_testName} ${_testSource} )
  TARGET_LINK_LIBRARIES( ${_testName} ${ARGN} )
  ADD_TEST( ${_testName} ${CMAKE_CURRENT_BINARY_DIR}/${_testName} )

ENDMACRO( OPENSYNC_ADD_TEST )
