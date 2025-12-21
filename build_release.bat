@echo off

echo Release building...

if not exist build (
	mkdir build
)

pushd build

set compiler_options=/nologo /FC
set preprocessor=-D"BUILD_TYPE_RELEASE=1"

cl %compiler_options% ..\src\win32_survivor.cpp /FoLastSurvivorReleaseX64 %preprocessor% /link user32.lib

popd