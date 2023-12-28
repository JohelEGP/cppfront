#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include "cpp2util.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
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
        if(!handle_) {
            cpp2::Default.report_violation(("failed to load DLL '" + path + "': " + get_last_error_()).c_str());
        }
    }

    ~dll() noexcept
    {
        if(handle_)
        {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle_));
#else
            dlclose(handle_);
#endif // _WIN32
        }
    }

    // Uncopyable
    dll(dll&) = delete;
    dll(dll const&) = delete;
    auto operator=(dll const&) -> dll& = delete;
    // Unmovable
    dll(dll&& from) = delete;
    auto operator=(dll&& from) -> dll& = delete;

    auto is_open() noexcept -> bool { return handle_; }

    template<typename T>
    auto get_alias(std::string const& name) noexcept -> T*
    {
#ifdef _WIN32
        auto symbol = GetProcAddress(static_cast<HMODULE>(handle_), name.c_str());
#else
        auto symbol = dlsym(handle_, name.c_str());
        if(!symbol)
        {
            // Some platforms export with additional underscore, so try that.
            auto const us_name = "_" + name;
            symbol = dlsym(handle_, us_name.c_str());
        }
#endif // _WIN32
        return function_cast_<T*>(symbol);
    }
private:
    void* handle_{nullptr};

    template<typename T>
    static auto function_cast_(auto ptr) noexcept -> T
    {
        using generic_function_ptr = void (*)(void);
        return reinterpret_cast<T>(reinterpret_cast<generic_function_ptr>(ptr));
    }

    static auto get_last_error_() noexcept -> std::string
    {
#ifdef _WIN32
        DWORD errorMessageID = GetLastError();
        if(errorMessageID == 0) {
            return {}; // No error message has been recorded
        }
        LPSTR messageBuffer = nullptr;
        auto size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorMessageID,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            (LPSTR)&messageBuffer,
            0,
            nullptr
        );
        std::string message(messageBuffer, static_cast<size_t>(size));
        LocalFree(messageBuffer);
        return message;
#else
        return std::string{dlerror()};
#endif // _WIN32
    }

};


struct load_metafunction_ret {
    std::function<void(type_declaration&)> metafunction;
    std::string error;
};


//  Load metafunction by opening DLL with OS APIs
//
//  The environment variable 'CPPFRONT_METAFUNCTION_LIBRARIES'
//  is read and interpreted as ':'-separated library paths
//  where a metafunction symbol is looked up at
auto load_metafunction(std::string const& name)
    -> load_metafunction_ret
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
    if (cpp1_libraries.empty()) {
        return {};
    }

    auto cpp1_name = "cpp2_metafunction_" + name;
    while (!cpp1_libraries.empty())
    {
        auto colon = cpp1_libraries.find(':');
        auto lib_path = cpp1_libraries.substr(0, colon);
        cpp1_libraries.remove_prefix(lib_path.size() + unsigned(colon != lib_path.npos));

        auto lib = std::make_shared<dll>(std::string(lib_path));
        if(!lib->is_open()) {
            continue;
        }

        if (auto* fun = lib->get_alias<void(void*)>(cpp1_name))
        {
            return {[
                fun = fun,
                lib = std::move(lib)
                ](type_declaration& t)
            {
                fun(static_cast<void*>(&t));
            }};
        }
    }

    return {{}, "metafunction not found in libraries " + std::string{cpp1_libraries_cstr}};
}

}
