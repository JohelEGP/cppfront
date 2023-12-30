
#ifndef REFLECT_IMPL_H_CPP2
#define REFLECT_IMPL_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"

#line 1 "reflect_impl.h2"

#line 238 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 250 "reflect_impl.h2"
class compiler_services_data;

#line 462 "reflect_impl.h2"
}

}


//=== Cpp2 type definitions and function declarations ===========================

#line 1 "reflect_impl.h2"

//  Copyright (c) Herb Sutter
//  SPDX-License-Identifier: CC-BY-NC-ND-4.0

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


//===========================================================================
//  Reflection and meta
//===========================================================================

#include "parse.h"
#include <cstdlib>
#include <functional>
#include <utility>
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
            Default.report_violation(("failed to load DLL '" + path + "': " + get_last_error()).c_str());
        }
    }

    ~dll() noexcept
    {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif // _WIN32
    }

    // Uncopyable
    dll(dll const&) = delete;
    auto operator=(dll const&) -> dll& = delete;
    // Unmovable
    dll(dll&&) = delete;
    auto operator=(dll&&) -> dll& = delete;

    template<typename T>
    auto get_alias(std::string const& name) noexcept -> T*
    {
#ifdef _WIN32
        auto symbol = GetProcAddress(static_cast<HMODULE>(handle_), name.c_str());
#else
        auto symbol = dlsym(handle_, name.c_str());
        if(!symbol)
        {
            //  Some platforms export with additional underscore, so try that
            auto const us_name = "_" + name;
            symbol = dlsym(handle_, us_name.c_str());
        }
#endif // _WIN32
        return function_cast<T*>(symbol);
    }
private:
    void* handle_ = nullptr;

    //  Properly convert the function pointer retrieved from GetProcAddress when building under mingw
    template<typename T>
    static auto function_cast(auto ptr) noexcept -> T
    {
        using generic_function_ptr = void (*)();
        return reinterpret_cast<T>(reinterpret_cast<generic_function_ptr>(ptr));
    }

    static auto get_last_error() noexcept -> std::string
    {
#ifdef _WIN32
        DWORD errorMessageID = GetLastError();
        if(errorMessageID == 0) {
            return {}; //  No error message has been recorded
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
        std::string message(messageBuffer, unsafe_narrow<std::size_t>(size));
        LocalFree(messageBuffer);
        return message;
#else
        return std::string{dlerror()};
#endif // _WIN32
    }

};


struct library
{
    std::string_view name;
    std::vector<std::string> symbols;
};

constexpr std::string_view symbol_prefix    = "cpp2_metafunction_";
constexpr std::string_view symbols_accessor = "cpp2_metafunction_get_symbol_names";

//  Load Cpp2 libraries with metafunctions by opening DLL with the OS API
//
//  The environment variable 'CPPFRONT_METAFUNCTION_LIBRARIES'
//  is read and interpreted as ':'-separated Cpp2 metafunction library paths
std::span<library> get_reachable_metafunction_symbols()
{
    static std::vector<library> res = []{
        std::vector<library> res;

        // FIXME: On Windows, using this approach with the system apis not set to utf8, will
        // break if a metafunction library contains unicode codepoints in its name, a proper
        // way to handle this would be to use _wgetenv and use wchar_t strings for the dll opening
        // function
        auto cpp1_libraries_cstr = std::getenv("CPPFRONT_METAFUNCTION_LIBRARIES");
        if (
            !cpp1_libraries_cstr
            || cpp1_libraries_cstr[0] == '\0'
            )
        {
            return res;
        }

        auto cpp1_libraries = std::string_view{cpp1_libraries_cstr};
        while (!cpp1_libraries.empty())
        {
            auto colon = cpp1_libraries.find(':');
            auto lib_path = cpp1_libraries.substr(0, colon);
            cpp1_libraries.remove_prefix(lib_path.size() + unsigned(colon != lib_path.npos));

            auto report_invalid_symbols_accessor = [&](std::string const& what) {
                Default.report_violation(
                    ("Cpp2 metafunction symbols accesor " + what + " (in '" + std::string{lib_path} + "')").c_str()
                );
            };
            auto lib = std::make_shared<dll>(std::string(lib_path));

            if (auto* get_symbols = lib->get_alias<char const**()>(std::string{symbols_accessor}))
            {
                res.push_back({lib_path, {}});
                auto c_strings = get_symbols();
                if (!c_strings || !*c_strings) {
                    report_invalid_symbols_accessor("returns no symbols");
                }

                for (; *c_strings; ++c_strings) {
                    auto symbol = res.back().symbols.emplace_back(*c_strings);
                    if (!symbol.starts_with(symbol_prefix)) {
                        report_invalid_symbols_accessor("returns invalid symbol '" + std::string{symbol} + "'");
                    }
                }
            }
            else
            {
                report_invalid_symbols_accessor("is missing");
            }
        }

        return res;
    }();

    return res;
}


struct lookup_res {
    std::string_view library;
    std::string_view symbol;
    std::string error;
};

struct load_metafunction_ret {
    std::function<void(type_declaration&)> metafunction;
    std::string error;
};

//  Load Cpp2 metafunction by opening DLL with the OS API
auto load_metafunction(
    std::string const& name,
    std::function<lookup_res(std::string const&)> lookup
    )
    -> load_metafunction_ret
{;
    auto [lib_path, cpp1_name, error] = lookup(name);

    if (!error.empty()) {
        return {{}, error};
    }

    auto lib = std::make_shared<dll>(std::string(lib_path));
    if (auto* fun = lib->get_alias<void(void*)>(std::string{cpp1_name}))
    {
        return {
            [
             fun = fun,
             lib = std::move(lib)
             ]
            (type_declaration& t)
                -> void
            {
                fun(static_cast<void*>(&t));
            },
            {}
        };
    }

    Default.report_violation(("failed to load metafunction '" + name + "' from '" + lib_path + "'").c_str());
    return {};
}

}

#line 238 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 243 "reflect_impl.h2"
//-----------------------------------------------------------------------
//
//  Compiler services data
//
//-----------------------------------------------------------------------
//

class compiler_services_data
 {
    //  Common data members
    //
    public: std::vector<error_entry>* errors; 
    public: int errors_original_size; 
    public: std::deque<token>* generated_tokens; 
    public: cpp2::parser parser; 
    public: std::string metafunction_name {}; 
    public: std::vector<std::string> metafunction_args {}; 
    public: bool metafunctions_used {false}; 

    //  Make function
    //
    public: [[nodiscard]] static auto make(
        std::vector<error_entry>* errors_, 
        std::deque<token>* generated_tokens_
    ) -> compiler_services_data;

#line 275 "reflect_impl.h2"
};

#line 278 "reflect_impl.h2"
//-----------------------------------------------------------------------
//
//  apply_metafunctions
//
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    type_declaration& rtype, 
    auto const& error, 
    auto const& lookup
    ) -> bool;

#line 396 "reflect_impl.h2"
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    function_declaration& rfunction, 
    auto const& error
    ) -> bool;

#line 462 "reflect_impl.h2"
}

}

#include "cpp2reflect.hpp"


//=== Cpp2 function definitions =================================================

#line 1 "reflect_impl.h2"

#line 238 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 264 "reflect_impl.h2"
    [[nodiscard]] auto compiler_services_data::make(
        std::vector<error_entry>* errors_, 
        std::deque<token>* generated_tokens_
    ) -> compiler_services_data

    {
        return { errors_, 
                cpp2::unsafe_narrow<int>(std::ssize(*cpp2::assert_not_null(errors_))), 
                generated_tokens_, 
                *cpp2::assert_not_null(errors_) }; 
    }

#line 282 "reflect_impl.h2"
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    type_declaration& rtype, 
    auto const& error, 
    auto const& lookup
    ) -> bool

{
    if (cpp2::Default.has_handler() && !(CPP2_UFCS(is_type)(n)) ) { cpp2::Default.report_violation(""); }

    //  Check for _names reserved for the metafunction implementation
    for ( 
          auto const& m : CPP2_UFCS(get_members)(rtype) ) 
    {
        CPP2_UFCS(require)(m, !(CPP2_UFCS(starts_with)(CPP2_UFCS(name)(m), "_")) || cpp2::cmp_greater(CPP2_UFCS(ssize)(CPP2_UFCS(name)(m)),1), 
                    "a type that applies a metafunction cannot have a body that declares a name that starts with '_' - those names are reserved for the metafunction implementation");
    }

    //  For each metafunction, apply it
    for ( 
         auto const& meta : n.metafunctions ) 
    {
        //  Convert the name and any template arguments to strings
        //  and record that in rtype
        auto name {CPP2_UFCS(to_string)((*cpp2::assert_not_null(meta)))}; 
        name = CPP2_UFCS(substr)(name, 0, CPP2_UFCS(find)(name, '<'));

        std::vector<std::string> args {}; 
        for ( 
             auto const& arg : CPP2_UFCS(template_arguments)((*cpp2::assert_not_null(meta))) ) 
            CPP2_UFCS(push_back)(args, CPP2_UFCS(to_string)(arg));

        CPP2_UFCS(set_metafunction_name)(rtype, name, args);

        //  Dispatch
        //
        if (name == "interface") {
            interface(rtype);
        }
        else {if (name == "polymorphic_base") {
            polymorphic_base(rtype);
        }
        else {if (name == "ordered") {
            ordered(rtype);
        }
        else {if (name == "weakly_ordered") {
            weakly_ordered(rtype);
        }
        else {if (name == "partially_ordered") {
            partially_ordered(rtype);
        }
        else {if (name == "copyable") {
            copyable(rtype);
        }
        else {if (name == "basic_value") {
            basic_value(rtype);
        }
        else {if (name == "value") {
            value(rtype);
        }
        else {if (name == "weakly_ordered_value") {
            weakly_ordered_value(rtype);
        }
        else {if (name == "partially_ordered_value") {
            partially_ordered_value(rtype);
        }
        else {if (name == "struct") {
            cpp2_struct(rtype);
        }
        else {if (name == "enum") {
            cpp2_enum(rtype);
        }
        else {if (name == "flag_enum") {
            flag_enum(rtype);
        }
        else {if (name == "union") {
            cpp2_union(rtype);
        }
        else {if (name == "print") {
            print(rtype);
        }
        else {if (name == "visible") {
            visible(rtype);
        }
        else {
{
auto const& load = load_metafunction(name, lookup);

#line 368 "reflect_impl.h2"
            if (load.metafunction) {
                CPP2_UFCS(metafunction)(load, rtype);
            }else {
                error("unrecognized metafunction name: " + name);
                if (CPP2_UFCS(find)(name, "::") == name.npos) {
                    error("currently supported built-in names are: interface, polymorphic_base, ordered, weakly_ordered, partially_ordered, copyable, basic_value, value, weakly_ordered_value, partially_ordered_value, struct, enum, flag_enum, union, print, visible");
                }
                if (!(CPP2_UFCS(empty)(load.error))) {
                    error(load.error);
                }
                return false; 
            }
}
#line 380 "reflect_impl.h2"
        }}}}}}}}}}}}}}}}

        if ((
            !(CPP2_UFCS(empty)(args)) 
            && !(CPP2_UFCS(arguments_were_used)(rtype)))) 

        {
            error(name + " did not use its template arguments - did you mean to write '" + name + " <" + CPP2_ASSERT_IN_BOUNDS(args, 0) + "> type' (with the spaces)?");
            return false; 
        }
    }

    return true; 
}

#line 396 "reflect_impl.h2"
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    function_declaration& rfunction, 
    auto const& error
    ) -> bool

{
    if (cpp2::Default.has_handler() && !(CPP2_UFCS(is_function)(n)) ) { cpp2::Default.report_violation(""); }

    //  Check for _names reserved for the metafunction implementation
//  for  rfunction.get_members()
//  do   (m)
//  {
//      m.require( !m.name().starts_with("_") || m.name().ssize() > 1,
//                  "a function that applies a metafunction cannot have a body that declares a name that starts with '_' - those names are reserved for the metafunction implementation");
//  }

    //  For each metafunction, apply it
    for ( 
         auto const& meta : n.metafunctions ) 
    {
        //  Convert the name and any template arguments to strings
        //  and record that in rfunction
        auto name {CPP2_UFCS(to_string)((*cpp2::assert_not_null(meta)))}; 
        name = CPP2_UFCS(substr)(name, 0, CPP2_UFCS(find)(name, '<'));

        std::vector<std::string> args {}; 
        for ( 
             auto const& arg : CPP2_UFCS(template_arguments)((*cpp2::assert_not_null(meta))) ) 
            CPP2_UFCS(push_back)(args, CPP2_UFCS(to_string)(arg));

        CPP2_UFCS(set_metafunction_name)(rfunction, name, args);

        //  Dispatch
        //
        if (name == "visible") {
            visible(rfunction);
        }
        else {
//          (load := load_metafunction(name))
//          if load.metafunction {
//              load.metafunction( rfunction );
//          } else {
                error("unrecognized metafunction name: " + name);
                error("currently supported built-in names are: visible");
//              if !load.error.empty() {
//                  error( load.error );
//              }
//              return false;
//          }
        }

        if ((
            !(CPP2_UFCS(empty)(args)) 
            && !(CPP2_UFCS(arguments_were_used)(rfunction)))) 

        {
            error(name + " did not use its template arguments - did you mean to write '" + name + " <" + CPP2_ASSERT_IN_BOUNDS(args, 0) + "> (...) -> ...' (with the spaces)?");
            return false; 
        }
    }

    return true; 
}

#line 462 "reflect_impl.h2"
}

}

#endif
