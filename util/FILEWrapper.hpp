#ifndef _BAZ_FILEWRAPPER_HPP
#define _BAZ_FILEWRAPPER_HPP
#include <iostream>

class FILEWrapper
{
public:
    FILEWrapper(const char* filename, const char* mode);
    ~FILEWrapper();
    FILEWrapper(const FILEWrapper& other) = delete;
    FILEWrapper& operator=(const FILEWrapper& other) = delete;

    FILE* Get();
    int Error();

private:
    FILE* filePointer;
};
#endif

