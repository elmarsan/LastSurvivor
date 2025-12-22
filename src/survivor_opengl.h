#pragma once

#include <gl/glcorearb.h>

struct OpenGL
{
    PFNGLCLEARCOLORPROC glClearColor;
    PFNGLCLEARPROC      glClear;
    PFNGLUSEPROGRAMPROC glUseProgram;
};