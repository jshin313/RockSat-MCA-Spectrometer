# makefile for Microsoft (Visual C++) NMAKE utility for compiling the application 
# usage: nmake /F capeMCAuart.mak
# or :   nmake /F capeMCAuart.mak clean
#
EXEFILE = capeMCAuart.exe
#					These are the header files for the application
HDRFILES = \
	packet0type.h \
	version.h
#	
#					Object files (targets of compilation)
OBJFILES = \
	capeMCAuart.obj
#
#					Must explicitly list all .lib files used
LIBFILES = \
	user32.lib \
	shell32.lib \
	winusb.lib \
	setupAPI.lib

######################## For Microsoft Visual Studio Community 2017 x64 compiler #######################
COMPILER = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl"
LINKER = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\link"
MSGTXT = VC++ 2017 64-bit BUILD WAS SUCCESSFUL!
################################## Visual Studio 2017 and SDK 10 ####################################
SDK_INCLUDE1="C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um"
SDK_INCLUDE2="C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared"
SDK_INCLUDE3="C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt"
SDK_INCLUDE4="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include"
SDK_LIB1="C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64"
SDK_LIB2="C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\ucrt\x64"
SDK_LIB3="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64"


CFLAGS  = /c /EHa /I$(SDK_INCLUDE1) /I$(SDK_INCLUDE2) /I$(SDK_INCLUDE3) /I$(SDK_INCLUDE4)
# /c   compile only, do not link
# /I$(SDK_INCLUDE3) another location of include files
# /Zi  include debugging information
# /EHa asynchronous exception handling allows catch(...) for ALL exceptions

LFLAGS  = /LIBPATH:$(SDK_LIB1) /LIBPATH:$(SDK_LIB2) /LIBPATH:$(SDK_LIB3)
# /LIBPATH location of libraries
# /DEBUG include debugging information

.SILENT:
all: $(EXEFILE)

#			Build command-line interface

$(EXEFILE) : $(OBJFILES)
    $(LINKER) /OUT:$(EXEFILE) /SUBSYSTEM:CONSOLE $(LFLAGS) $(OBJFILES) $(LIBFILES)
    echo $(EXEFILE) $(MSGTXT)

#			Compile C source files
.c.obj :
	$(COMPILER) $(CFLAGS) $<

#			Compile C++ source files
.cpp.obj :
	$(COMPILER) $(CFLAGS) $<

#			Dependencies:
#			 (if a header file changes, just recompile everthing)
*.c *.cpp: $(HDRFILES)

clean :
   del *.obj *.bak *.pdb *.ilk *.suo
   echo CapeMCA CLEANED
