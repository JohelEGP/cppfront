
#ifndef REFLECT_IMPL_H_CPP2
#define REFLECT_IMPL_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"

#line 1 "reflect_impl.h2"

#line 23 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 35 "reflect_impl.h2"
class compiler_services_data;

#line 244 "reflect_impl.h2"
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

#include "reflect_impl_load_metafunction.h"

#include "parse.h"

#line 23 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 28 "reflect_impl.h2"
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

#line 60 "reflect_impl.h2"
};

#line 63 "reflect_impl.h2"
//-----------------------------------------------------------------------
//
//  apply_metafunctions
//
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    type_declaration& rtype, 
    auto const& error
    ) -> bool;

#line 178 "reflect_impl.h2"
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    function_declaration& rfunction, 
    auto const& error
    ) -> bool;

#line 244 "reflect_impl.h2"
}

}

#include "cpp2reflect.hpp"


//=== Cpp2 function definitions =================================================

#line 1 "reflect_impl.h2"

#line 23 "reflect_impl.h2"
namespace cpp2 {

namespace meta {

#line 49 "reflect_impl.h2"
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

#line 67 "reflect_impl.h2"
[[nodiscard]] auto apply_metafunctions(
    declaration_node& n, 
    type_declaration& rtype, 
    auto const& error
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
auto const& load = load_metafunction(name);

#line 152 "reflect_impl.h2"
            if (load.metafunction) {
                CPP2_UFCS(metafunction)(load, rtype);
            }else {
                error("unrecognized metafunction name: " + name);
                error("currently supported built-in names are: interface, polymorphic_base, ordered, weakly_ordered, partially_ordered, copyable, basic_value, value, weakly_ordered_value, partially_ordered_value, struct, enum, flag_enum, union, print, visible");
                if (!(CPP2_UFCS(empty)(load.error))) {
                    error(load.error);
                }
                return false; 
            }
}
#line 162 "reflect_impl.h2"
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

#line 178 "reflect_impl.h2"
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

#line 244 "reflect_impl.h2"
}

}

#endif
