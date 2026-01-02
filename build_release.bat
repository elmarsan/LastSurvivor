@echo off

echo Release building...

if not exist build (
	mkdir build
)

pushd build

del *.pdb > NUL 2> NUL

set compiler_options=/nologo /FC
set preprocessor=-D"BUILD_TYPE_RELEASE=1" -D"NOMINMAX=1" -D"WIN32_LEAN_AND_MEAN=1"
set includes=-I..\external
set libs=user32.lib opengl32.lib gdi32.lib ole32.lib

cl %compiler_options% %includes% ..\src\win32_survivor.cpp /FoLastSurvivorReleaseX64 %preprocessor% /link %libs%

popd