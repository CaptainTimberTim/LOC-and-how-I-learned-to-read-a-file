@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=C:\Workspace\Programming\CPP_Projects\LOC;C:\Workspace\Programming\CPP_Projects\LOC\code;%path%
"editor"