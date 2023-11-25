


all:	ChkExe.exe



ChkExe.exe : ChkExe.obj ChkExe.res
	cl ChkExe.obj ChkExe.res kernel32.lib user32.lib gdi32.lib \
						comdlg32.lib shell32.lib

ChkExe.obj : ChkExe.cpp ChkExe.h
	cl /c ChkExe.cpp

ChkExe.res : ChkExe.rc ChkExe.h
	rc ChkExe.rc



