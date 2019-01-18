@echo off
set "common_compiler_flags= /MT /Gm- /FC /W4 /wd4100 /Od /Oi /std:c++17 /nologo /GR- /MP /Zo /Zf /Z7 /Ia:\src\ /DEBUG"
set "engine_compiler_flags= /DANT_DEBUG /DANT_VERSION=0"
set "src=a:\src"

IF NOT EXIST a:\build mkdir a:\build
pushd a:\build

del /q *

echo Compilation started at %time%

cl %common_compiler_flags% %engine_compiler_flags% /I%VULKAN_SDK%\include\ /Fmwin32_ant.map %src%\win32_ant.cpp /link /LIBPATH:%VULKAN_SDK%\Lib\ /DYNAMICBASE "vulkan-1.lib" /INCREMENTAL:NO user32.lib kernel32.lib /opt:ref /out:win32_ant.exe

IF EXIST a:\build\win32_ant.exe echo Compilation finished at %time%

popd
