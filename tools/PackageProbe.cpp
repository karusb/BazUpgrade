#include <BazPO.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace
{
struct Library
{
#ifdef _WIN32
    HMODULE handle = nullptr;
    ~Library()
    {
        if (handle)
            FreeLibrary(handle);
    }
#else
    void *handle = nullptr;
    ~Library()
    {
        if (handle)
            dlclose(handle);
    }
#endif
};
} // namespace
int main(int argc, const char *argv[])
{
    try
    {
        BazPO::Cli cli(argc, argv, "Verify the packaged C ABI");
        cli.option("--library", "", "Absolute packaged SDK library").withMaxValueCount(1).mandatory();
        cli.parse();
        auto path = std::filesystem::absolute(std::filesystem::u8path(cli.valueAs<std::string>("--library")));
        Library library;
#ifdef _WIN32
        library.handle = LoadLibraryExW(path.c_str(), nullptr,
                                        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
        library.handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        if (!library.handle)
            throw std::runtime_error("Cannot load packaged SDK");
        for (const char *name : {"baz_client_create", "baz_client_run", "baz_client_cancel",
                                 "baz_client_destroy", "baz_string_free"})
        {
#ifdef _WIN32
            if (!GetProcAddress(library.handle, name))
                throw std::runtime_error(std::string("Missing C ABI symbol: ") + name);
#else
            if (!dlsym(library.handle, name))
                throw std::runtime_error(std::string("Missing C ABI symbol: ") + name);
#endif
        }
        std::cout << "Packaged C ABI verified\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
