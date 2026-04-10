#pragma once

internal PLATFORM_LOGF(LogMsg)
{
#if BUILD_TYPE_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
#endif
}

PLATFORM_FILE_READ_ENTIRE(FileReadEntire)
{
    FileReadResult result = { 0 };

    FILE* file = fopen(filename, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        if (size != -1L)
        {
            result.contentSize = (size_t)size;
            result.content     = malloc(sizeof(u8) * size);
            fseek(file, 0, SEEK_SET);
            fread(result.content, 1, result.contentSize, file);
            fclose(file);
        }
        else
        {
            Log("Unable to reach the end of the file '%s'", filename);
        }
    }
    else
    {
        Log("Unable to open file '%s'", filename);
    }

    return result;
}

PLATFORM_FILE_FREE(FileFree)
{
    if (fileContent)
    {
        free(fileContent);
    }
}

PLATFORM_FILE_WRITE_ENTIRE(FileWriteEntire)
{
    FILE* file = fopen(filename, "wb");
    if (file)
    {
        size_t writenCount = fwrite(fileContent, size, 1, file);
        if (writenCount != 1)
        {
            Log("File '%s' was partially written", filename);
        }
        fclose(file);
    }
    else
    {
        Log("Unable to open file '%s'", filename);
    }
}