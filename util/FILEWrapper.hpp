#ifndef _BAZ_FILEWRAPPER_HPP
#define _BAZ_FILEWRAPPER_HPP
#include <iostream>

class FILEWrapper
{
public:
    FILEWrapper(const char* filename, const char* mode);
    ~FILEWrapper();

    FILE* Get();
    int Error();

private:
    FILE* filePointer;
};
#endif

