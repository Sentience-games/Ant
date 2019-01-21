@echo off

set "common_compiler_flags= /MT /Gm- /FC /W4 /wd4100 /Od /Oi /std:c++17 /nologo /GR- /MP /Zo /Zf /Z7 /Ia:\src\ /DEBUG"
set "common_linker_flags= /INCREMENTAL:NO user32.lib kernel32.lib /opt:ref"
set "engine_compiler_flags= /DANT_DEBUG /DANT_VERSION=0"
set "vulkan_compiler_flags= /I%VULKAN_SDK%\include\"
set "vulkan_linker_flags= /LIBPATH:%VULKAN_SDK%\Lib\ /DYNAMICBASE vulkan-1.lib"
set "src=a:\src"

IF NOT EXIST a:\build mkdir a:\build
pushd a:\build

del /q *

echo Compilation started at %time%

cl %common_compiler_flags% %engine_compiler_flags% %vulkan_compiler_flags% /Fmant.map %src%\ant.cpp /LD /link /DLL %vulkan_linker_flags% %common_linker_flags% /EXPORT:GameInit /EXPORT:GameUpdate /out:ant.dll
cl %common_compiler_flags% %engine_compiler_flags% %vulkan_compiler_flags% /Fmwin32_ant.map %src%\win32_ant.cpp /link %vulkan_linker_flags% %common_linker_flags% /out:win32_ant.exe

IF EXIST a:\build\win32_ant.exe echo Compilation finished at %time%

popd
