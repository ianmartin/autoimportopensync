# Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>

IF ( WIN32 )
	#ADD_DEFINITIONS(  )
ENDIF ( WIN32 )

IF ( CMAKE_COMPILER_IS_GNUCC ) 
	ADD_DEFINITIONS( -fvisibility=hidden )
ENDIF ( CMAKE_COMPILER_IS_GNUCC ) 

IF (CMAKE_SYSTEM MATCHES "SunOS-5*.")	
	ADD_DEFINITIONS( -xldscope=hidden )
ENDIF (CMAKE_SYSTEM MATCHES "SunOS-5*.")	
