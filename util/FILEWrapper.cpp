#include "FILEWrapper.hpp"

FILEWrapper::FILEWrapper(const char* filename, const char* mode)
{
    filePointer = fopen(filename, mode);
}

FILEWrapper::~FILEWrapper()
{
    fclose(filePointer);
}

FILE* FILEWrapper::Get()
{
    return filePointer;
}

int FILEWrapper::Error()
{
    if (!filePointer)
        return 1;
    else
        return ferror(filePointer);
}
