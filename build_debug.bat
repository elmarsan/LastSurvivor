@echo off

echo Debug building...

if not exist build (
	mkdir build
)

pushd build

set compiler_options=/nologo /FC /Zi
set preprocessor=-D"BUILD_TYPE_DEBUG=1"

cl %compiler_options% ..\src\win32_survivor.cpp /FoLastSurvivorDebugX64 %preprocessor% /link user32.lib

popd