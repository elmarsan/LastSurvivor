#pragma once

enum PlatformErrorType
{
    PlatformErrorTypeFatal,
    PlatformErrorTypeNonFatal
};

#define PLATFORM_ERROR_MESSAGE(name) void name(PlatformErrorType errorType, const char* message)
typedef PLATFORM_ERROR_MESSAGE(PlatformErrorMessage);

struct PlatformAPI
{
    PlatformErrorMessage* ErrorMessage;
};