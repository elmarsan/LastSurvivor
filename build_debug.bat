@echo off

echo Debug building...

:: Get current epoch unix timestamp
for /f %%i in ('powershell -NoProfile -Command "[int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds()"') do set unix_epoch=%%i

if not exist build (
	mkdir build
)

pushd build

del *.pdb > NUL 2> NUL

set compiler_options=/nologo /FC /Zi
set preprocessor=-D"BUILD_TYPE_DEBUG=1" -D"NOMINMAX=1" -D"WIN32_LEAN_AND_MEAN=1"
set includes=-I..\external
set libs=user32.lib opengl32.lib gdi32.lib

echo Building Executable...
cl %compiler_options% %includes% ..\src\win32_survivor.cpp /FoLastSurvivorDebugX64 %preprocessor% /link %libs%

echo Building DLL...
set linker_options=-incremental:no /PDB:Survivor_%unix_epoch%.pdb -EXPORT:GameUpdateAndRender
cl %compiler_options% /LD %includes% ..\src\survivor.cpp /FoSurvivor /link %linker_options%

popd