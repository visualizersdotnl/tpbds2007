@echo off

REM Pak file(s), executable & Bass DLL.
copy secretdesire.wad release
copy VS9\Release-NoExceptions\VS9.exe release
copy 3rdparty\bass\bass.dll release

REM Soundtrack (only content file not contained in the archive(s)).
cd release
mkdir content
copy ..\content\desire.mp3 content\desire.mp3
cd ..

REM Done!
echo Don't forget to supply the D3DX DLL from the SDK you've built with as well!
pause
