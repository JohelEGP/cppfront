
//  Copyright (c) Herb Sutter
//  SPDX-License-Identifier: CC-BY-NC-ND-4.0

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#ifdef _MSC_VER
#pragma warning(disable: 4456)
#endif

#include "cpp2util.h"


//===========================================================================
//  Common types
//===========================================================================

#ifndef CPP2_COMMON_H
#define CPP2_COMMON_H

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cctype>
#include <iomanip>
#include <compare>
#include <algorithm>
#include <unordered_map>

namespace cpp2 {

//-----------------------------------------------------------------------
//
//  source_line: represents a source code line
//
//-----------------------------------------------------------------------
//
struct source_line
{
    std::string text;

    enum class category { empty, preprocessor, comment, import, cpp1, cpp2, rawstring };
    category cat;

    bool all_tokens_are_densely_spaced = true; // to be overridden in lexing if they're not

    source_line(
        std::string_view t = {},
        category         c = category::empty
    );

    auto indent() const
        -> int;

    auto prefix() const
        -> std::string;
};


using lineno_t = int32_t;
using colno_t  = int32_t;   // not int16_t... encountered >80,000 char line during testing

struct source_position
{
    lineno_t    lineno;     // one-based offset into program source
    colno_t     colno;      // one-based offset into line

    source_position(lineno_t l = 1, colno_t  c = 1 );

    auto operator<=>(source_position const&) const = default;

    auto to_string() const
        -> std::string;
};

struct comment
{
    enum class comment_kind { line_comment = 0, stream_comment };

    comment_kind    kind;
    source_position start;
    source_position end;
    std::string     text;

    mutable bool    dbg_was_printed = false;
};

struct string_parts {
    struct cpp_code   { std::string text; };
    struct raw_string { std::string text; };
    enum adds_sequences { no_ends = 0, on_the_beginning = 1, on_the_end = 2, on_both_ends = 3 };

    string_parts(const std::string& beginseq,
                 const std::string& endseq,
                 adds_sequences     strateg);

    void add_code(const std::string& text);
    void add_string(const std::string& text);
    void add_string(const std::string_view& text);

    void clear();

    auto generate() const -> std::string;

    auto is_expanded() const -> bool;

private:
    std::string     begin_seq;
    std::string     end_seq;
    adds_sequences  strategy;
    std::vector<std::variant<raw_string, cpp_code>> parts;

    struct begin_visit {
        std::string begin_seq;
        adds_sequences strategy;

        auto operator()(const raw_string& part) const -> std::string;
        auto operator()(const cpp_code& part) const -> std::string;
    };

    struct end_visit {
        std::string end_seq;
        adds_sequences strategy;
        auto operator()(const raw_string&) const -> std::string;
        auto operator()(const cpp_code&) const -> std::string;
    };

    struct generator_visit {
        std::string begin_seq;
        std::string end_seq;

        auto operator()(const raw_string&, const cpp_code& part ) const -> std::string;
        auto operator()(const cpp_code&, const raw_string& part ) const -> std::string;
        auto operator()(const raw_string&, const raw_string& part ) const -> std::string;
        auto operator()(const cpp_code&, const cpp_code& part ) const -> std::string;
    };
};

struct raw_string
{
    source_position start;
    std::string     text;
    std::string     opening_seq;
    std::string     closing_seq;
    bool            should_interpolate = false;
};

struct multiline_raw_string
{
    std::string     text;
    source_position end = {0, 0};
};

//-----------------------------------------------------------------------
//
//  error: represents a user-readable error message
//
//-----------------------------------------------------------------------
//
struct error_entry
{
    source_position where;
    std::string     msg;
    bool            internal = false;
    bool            fallback = false;   // only emit this message if there was nothing better

    error_entry(
        source_position  w,
        std::string_view m,
        bool             i = false,
        bool             f = false
    );

    auto operator==(error_entry const& that)
        -> bool;

    auto print(auto& o, std::string const& file) const
        -> void;
};


//-----------------------------------------------------------------------
//
//  Digit classification, with '\'' digit separators
//
//-----------------------------------------------------------------------
//

//G binary-digit:
//G     one of '0' '1'
//G
auto is_binary_digit(char c)
    -> bool;

//G digit: one of
//G     binary-digit
//G     one of '2' '3' '4' '5' '6' '7' '8' '9'
//G
auto is_digit(char c)
    -> bool;

//G hexadecimal-digit:
//G     digit
//G     one of 'A' 'B' 'C' 'D' 'E' 'F'
//G
auto is_hexadecimal_digit(char c)
    -> bool;

//G nondigit:
//G     one of 'a'..'z'
//G     one of 'A'..'Z'
//G     _
//G
auto is_nondigit(char c)
    -> bool;;

//G identifier-start:
//G     nondigit
//G
auto is_identifier_start(char c)
    -> bool;

//G identifier-continue:
//G     digit
//G     nondigit
//G
auto is_identifier_continue(char c)
    -> bool;

//G identifier:
//G     '__identifier__' keyword    [Note: without whitespace before the keyword]
//G     identifier-start
//G     identifier identifier-continue
//G     'operator' operator
//G
auto starts_with_identifier(std::string_view s)
    -> int;;


//  Helper to allow one of the above or a digit separator
//  Example:    is_separator_or( is_binary_digit (c) )
//
auto is_separator_or(auto pred, char c)
    -> bool;


//  Bool to string
//
template<typename T>
    requires std::is_same_v<T, std::string>
auto _as(bool b)
    -> T;


//  Explicit cast
//
template<typename T>
auto _as(auto x)
    -> T;


//  String path prefix from filename
//
auto strip_path(std::string const& file)
    -> std::string;


//-----------------------------------------------------------------------
//
//  Misc helpers
//
//-----------------------------------------------------------------------
//
auto replace_all(std::string& s, std::string_view what, std::string_view with);


auto to_upper(char c)
    -> char;


auto to_upper_and_underbar(std::string_view s)
    -> std::string;


auto is_empty_or_a_decimal_number(std::string_view s)
    -> bool;


auto starts_with(
    std::string const& s,
    std::string_view   sv
)
    -> bool;


auto contains(
    auto const& range,
    auto const& value
)
    -> bool;

auto contains(
    std::string const& s,
    auto const&        value
)
    -> bool;


//  In keep trying to write string+string_view, and it ought to Just Work without
//  the current workarounds. Not having that is a minor impediment to using safe
//  and efficient string_views, which we should be encouraging. So for my own use
//  and to remove that minor impediment to writing safe and efficient code, I'm
//  just going to add this until we get P2591 in C++26(?) -- See: wg21.link/p2591
//
template<class charT, class traits, class Allocator>
[[nodiscard]] constexpr auto operator+(
    std::basic_string<charT, traits, Allocator> lhs,
    std::type_identity_t<std::basic_string_view<charT, traits>> rhs
    )
    -> std::basic_string<charT, traits, Allocator>;

template<class charT, class traits, class Allocator>
[[nodiscard]] constexpr auto operator+(
    std::type_identity_t<std::basic_string_view<charT, traits>> lhs,
    std::basic_string<charT, traits, Allocator> rhs
    )
    -> std::basic_string<charT, traits, Allocator>;


//-----------------------------------------------------------------------
//
//  Command line handling
//
//-----------------------------------------------------------------------
//

class cmdline_processor
{
    bool help_requested = false;

    struct arg
    {
        int pos;
        std::string text;

        arg(int p, char* t);
    };
    std::vector<arg> args;

    using callback0 = void (*)();
    using callback1 = void (*)(std::string const&);
    struct flag
    {
        int         group = 0;
        std::string name;
        int         unique_prefix = 0;
        std::string description;
        callback0   handler0;
        callback1   handler1;
        std::string synonym;
        bool        opt_out;

        flag(int g, std::string_view n, std::string_view d, callback0 h0, callback1 h1, std::string_view s, bool o);
    };
    std::vector<flag> flags;
    int max_flag_length = 0;

    std::unordered_map<int, std::string> labels = {
        { 2, "Additional dynamic safety checks and contract information" },
        { 4, "Support for constrained target environments" },
        { 9, "Other options" }
    };

    //  Define this in the main .cpp to avoid bringing <iostream> into the headers,
    //  so that we can't accidentally start depending on iostreams in the compiler body
    static auto print(std::string_view, int width = 0)
        -> void;

public:
    auto process_flags()
        -> void;

    auto print_help()
        -> void;

    auto add_flag(
        int              group,
        std::string_view name,
        std::string_view description,
        callback0        handler0,
        callback1        handler1,
        std::string_view synonym,
        bool             opt_out
    )
        -> void;
    struct register_flag {
        register_flag(
            int              group,
            std::string_view name,
            std::string_view description,
            callback0        handler0,
            callback1        handler1 = {},
            std::string_view synonym  = {},
            bool             opt_out  = false
        );
    };

    auto set_args(
        int   argc,
        char* argv[]
    )
        -> void;

    auto help_was_requested()
        -> bool;

    auto arguments()
        -> std::vector<arg>&;

    //  This is used only by the owner of the 'main' branch
    //  to generate stable build version strings
    auto gen_version()
        -> void;

    auto print_version()
        -> void;

};

}

#endif
