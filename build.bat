@echo off

set "common_compiler_flags= /MT /Gm- /FC /W4 /wd4100 /Od /Oi /std:c++17 /nologo /GR- /MP /Zo /Zf /Z7 /Ia:\src\ /DEBUG"
set "common_linker_flags= /INCREMENTAL:NO user32.lib kernel32.lib /opt:ref"
set "engine_compiler_flags= /DANT_DEBUG /DANT_VERSION=0 /DANT_PLATFORM_WINDOWS"
set "engine_linker_flags= renderer.lib"
set "vulkan_compiler_flags= /I%VULKAN_SDK%\include\"
set "src=a:\src"

IF NOT EXIST a:\build mkdir a:\build
pushd a:\build

echo Build started at %time%

REM Building the renderer
if [%1] == [renderer_only] (
del /s renderer.* > nul 2>&1
cl %common_compiler_flags% %vulkan_compiler_flags% /DPLATFORM_WINDOWS /DDEBUG_MODE /Fmrenderer.map %src%\renderer\renderer.cpp /LD /link /DLL /PDB:renderer.pdb %common_linker_flags% /out:renderer.dll
echo Build finished at %time%
GOTO exit
)

REM Building the game for hot reloading
IF [%1] == [game_only] (
del /s ant.* > nul 2>&1
del /s ant_******.pdb > nul 2>&1
del /s ant_loaded.dll > nul 2>&1
cl %common_compiler_flags% %engine_compiler_flags% %vulkan_compiler_flags% /Fmant.map %src%\ant.cpp /LD /link /DLL /PDB:ant_%time:~3,2%%time:~6,2%%time:~9,2%.pdb %common_linker_flags% %engine_linker_flags% /out:ant.dll
echo Build finished at %time%
GOTO exit
)

del /s ant.* 1>nul
del /s ant_******.pdb > nul 2>&1
del /s win32_ant.* > nul 2>&1
del /s ant_loaded.dll > nul 2>&1

REM Building the engine and the game
:compile_engine

cl %common_compiler_flags% %engine_compiler_flags% %vulkan_compiler_flags% /Fmant.map %src%\ant.cpp /LD /link /DLL /PDB:ant.pdb %common_linker_flags% %engine_linker_flags% /out:ant.dll
cl %common_compiler_flags% %engine_compiler_flags% %vulkan_compiler_flags% /Fmwin32_ant.map %src%\win32_ant.cpp /link /PDB:win32_ant.pdb %common_linker_flags% %engine_linker_flags% /out:win32_ant.exe

IF EXIST a:\build\win32_ant.exe echo Build finished at %time%

:exit

popd
