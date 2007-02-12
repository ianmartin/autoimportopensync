# Microsoft Developer Studio Project File - Name="scapiwin32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=scapiwin32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scapiwin32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scapiwin32.mak" CFG="scapiwin32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scapiwin32 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "scapiwin32 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "scapiwin32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\src\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_UNICODE" /D "UNICODE" /D "NO_CRIT_MSGBOX" /YX /FD /c
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "scapiwin32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\output\win32"
# PROP Intermediate_Dir "..\..\output\win32"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\src\include" /D "WIN32" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "NO_CRIT_MSGBOX" /YX /FD /GZ /c
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "scapiwin32 - Win32 Release"
# Name "scapiwin32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "http"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\c++\http\common\HTTPHeader.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\http\common\Proxy.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\http\win32\TransportAgent.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\http\common\URL.cpp"
# End Source File
# End Group
# Begin Group "spds"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\c++\spds\common\Config.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SPDSUtils.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncItem.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncItemHolder.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncItemStatus.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncItemStatusHolder.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncManager.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncManagerFactory.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncMap.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncMLBuilder.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncMLProcessor.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncSource.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spds\common\SyncSourceConfig.cpp"
# End Source File
# End Group
# Begin Group "spdm"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\c++\spdm\common\Base64.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\common\DeviceManager.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\win32\DeviceManagerFactory.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\common\ManagementNode.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\common\SPDMUtils.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\win32\Win32DeviceManager.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\spdm\win32\Win32ManagementNode.cpp"
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\c++\common\Error.cpp"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\common\Log.cpp"
# End Source File
# End Group
# Begin Group "ptypes"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\src\c++\ptypes\patomic.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\pfatal.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\pmem.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\pobjlist.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\pstring.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\pstrmanip.cxx"
# End Source File
# Begin Source File

SOURCE="..\..\src\c++\ptypes\punknown.cxx"
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
