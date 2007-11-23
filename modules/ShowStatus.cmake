# Shows status of a option variable
#
# usage: SHOW_STATUS( <VariableName> <MessageText> )
#
# Copyright (C) 2007 Bjoern Ricks <b.ricks@fh-osnabrueck.de> 
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

MACRO( SHOW_STATUS _name _message )
	IF( DEFINED ${_name} )
		MESSAGE( STATUS "${_message}\t\t${${_name}}" )
	ELSE( DEFINED ${_name} )
		MESSAGE( STATUS "${_message}\t\tOFF" )
	ENDIF( DEFINED ${_name})
ENDMACRO( SHOW_STATUS ) 
