#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include "cpp2util.h"

#ifdef _WIN32
#include <libloaderapi.h>
#else
#include <dlfcn.h>
#endif // _WIN32

namespace cpp2::meta {

class dll
{
public:
    dll(std::string const& path)
    {
#ifdef _WIN32
        handle_ = static_cast<void*>(LoadLibraryA(path.c_str()));
#else
        handle_ = static_cast<void*>(dlopen(path.c_str(), RTLD_NOW|RTLD_LOCAL));
#endif // _WIN32
        // TODO: log if the dll could not be open?
    }

    ~dll() noexcept
    {
        if(handle_ == nullptr);
            return;
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif // _WIN32
    }

    // Uncopyable
    dll(dll&) = delete;
    dll(dll const&) = delete;
    auto operator=(dll const&) -> dll& = delete;

    // Movable
    dll(dll&& from) noexcept
    {
        handle_ = from.handle_;
        from.handle_ = nullptr;
    }

    auto operator=(dll&& from) noexcept -> dll&
    {
        handle_ = from.handle_;
        from.handle_ = nullptr;
        return *this;
    }

    auto is_open() noexcept -> bool { return handle_ != nullptr; }

    template<typename T>
    auto get_alias(std::string const& name) noexcept -> T*
    {
#ifdef _WIN32
        auto symbol = GetProcAddress(static_cast<HMODULE>(handle_), name.c_str());
#else
        auto symbol = dlsym(handle_, name.c_str());
        if(symbol == nullptr)
        {
            // Some platforms export with additional underscore, so try that.
            auto const us_name = "_" + name;
            symbol = dlsym(handle_, us_name.c_str());
        }
#endif // _WIN32
        // TODO: log if the symbol is not found?
        return function_cast<T*>(symbol);
    }
private:
    void* handle_{nullptr};

    template<typename T>
    static T function_cast(auto ptr) {
        using generic_function_ptr = void (*)(void);
        return reinterpret_cast<T>(reinterpret_cast<generic_function_ptr>(ptr));
    }

};


//  Load metafunction by opening DLL with OS APIs
//
//  The ':'-separated library paths
//  are read from the environment variable
//  'CPPFRONT_METAFUNCTION_LIBRARIES'
auto load_metafunction(std::string const& name) -> std::function<void(type_declaration&)>
{
    // FIXME: On Windows, using this approach with the system apis not set to utf8, will
    // break if a metafunction library contains unicode codepoints in its name, a proper
    // way to handle this would be to use _wgetenv and use wchar_t strings for the dll opening
    // function
    auto cpp1_libraries_cstr = std::getenv("CPPFRONT_METAFUNCTION_LIBRARIES");
    if (!cpp1_libraries_cstr) {
        return {};
    }

    auto cpp1_libraries = std::string_view{cpp1_libraries_cstr};
    auto cpp1_name = "cpp2_metafunction_" + name;

    while (!cpp1_libraries.empty())
    {
        auto colon = cpp1_libraries.find(':');
        auto lib_path = cpp1_libraries.substr(0, colon);
        cpp1_libraries.remove_prefix(lib_path.size() + unsigned(colon != lib_path.npos));

        auto lib = std::make_shared<dll>(std::string(lib_path));
        if(!lib->is_open())
            continue;

        if (auto* fun = lib->get_alias<void(void*)>(cpp1_name); fun != nullptr)
        {
            return [
                fun = fun,
                lib = lib
                ](type_declaration& t)
            {
                fun(static_cast<void*>(&t));
            };
        }
    }

    return {};
}

}
