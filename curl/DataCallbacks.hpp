#ifndef _BAZ_DATACALLBACKS_HPP
#define _BAZ_DATACALLBACKS_HPP
#include <string>
#include <iostream>

extern size_t callback(void* ptr, size_t size, size_t nmemb, std::string* data);
extern size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
#endif
