@echo off

set "common_compiler_flags= /diagnostics:column /MT /Gm- /FC /W4 /wd4100 /wd4201 /Od /Oi /std:c++17 /nologo /GR- /MP /Zo /Zf /Z7 /Ia:\src\ /DEBUG"
set "common_linker_flags= /INCREMENTAL:NO user32.lib kernel32.lib Gdi32.lib /opt:ref"
set "engine_compiler_flags= /DANT_DEBUG /DANT_VERSION=1 /DANT_PLATFORM_WINDOWS"
set "src=a:\src"

IF NOT EXIST a:\build mkdir a:\build
IF NOT EXIST a:\run_tree mkdir a:\run_tree
pushd a:\build

echo Build started at %time%
echo.

REM Building the game for hot reloading
IF [%1] == [game_only] (
del /Q "game.*"			   > nul 2>&1
del /Q "game_******.pdb"	  > nul 2>&1
del /Q "game_loaded.dll"	  > nul 2>&1
del /Q "../run_tree/game.dll" > nul 2>&1
cl %common_compiler_flags% %engine_compiler_flags% /Fmgame.map %src%\game.cpp /LD /link /DLL /PDB:game_%time:~3,2%%time:~6,2%%time:~9,2%.pdb %common_linker_flags% /EXPORT:GameUpdateAndRender /out:game.dll
echo Build finished at %time%
GOTO move_files_to_run_tree
)

REM Building the engine and the game
:compile_engine

del /Q "game.*"					  > nul 2>&1
del /Q "game_******.pdb"			 > nul 2>&1
del /Q "win32_ant.*"				> nul 2>&1
del /Q "game_loaded.dll"			 > nul 2>&1
del /Q "../run_tree/win32_ant.exe"  > nul 2>&1
del /Q "../run_tree/game.dll"		> nul 2>&1
del /Q "../run_tree/game_loaded.dll" > nul 2>&1

cl %common_compiler_flags% %engine_compiler_flags% /Fmgame.map %src%\game.cpp /LD /link /DLL /PDB:game.pdb %common_linker_flags% /EXPORT:GameUpdateAndRender /out:game.dll

IF NOT EXIST a:\build\game.dll GOTO failed

cl %common_compiler_flags% %engine_compiler_flags% /Fmwin32_ant.map %src%\win32_ant.cpp /link /PDB:win32_ant.pdb %common_linker_flags% /out:win32_ant.exe

IF NOT EXIST a:\build\win32_ant.exe GOTO failed

echo.
echo Build finished at %time%
GOTO move_files_to_run_tree

REM Move the built files to the run_tree directory
:move_files_to_run_tree
copy /Y "a:\build\win32_ant.exe" "a:\run_tree\win32_ant.exe" > nul 2>&1
copy /Y "a:\build\game.dll" "a:\run_tree\game.dll" > nul 2>&1
GOTO exit

:failed
echo.
echo Build failed at %time%

:exit

popd
