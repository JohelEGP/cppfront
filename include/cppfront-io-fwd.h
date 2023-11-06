
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
//  Source loader
//===========================================================================

#ifndef CPP2_IO_H
#define CPP2_IO_H

#include "cppfront-common-fwd.h"
#include <fstream>
#include <ostream>
#include <iterator>
#include <cctype>


namespace cpp2 {

//---------------------------------------------------------------------------
//  move_next: advances i as long as p(line[i]) is true or the end of line
//
//  line    current line being processed
//  i       current index
//  p       predicate to apply
//
auto move_next(
    std::string const& line,
    int&               i,
    auto               p
)
    -> bool;


//---------------------------------------------------------------------------
//  peek_first_non_whitespace: returns the first non-whitespace char in line
//
//  line    current line being processed
//
auto peek_first_non_whitespace(std::string const& line)
    -> char;


//---------------------------------------------------------------------------
//  is_preprocessor: returns whether this is a preprocessor line starting
//  with #, and whether it will be followed by another preprocessor line
//
//  line        current line being processed
//  first_line  whether this is supposed to be the first line (start with #)
//
struct is_preprocessor_ret {
    bool is_preprocessor;
    bool has_continuation;
};
auto is_preprocessor(
    std::string const& line,
    bool               first_line
)
    -> is_preprocessor_ret;


//---------------------------------------------------------------------------
//  starts_with_import: returns whether the line starts with "import"
//
//  line    current line being processed
//
auto starts_with_import(std::string const& line)
    -> bool;


//---------------------------------------------------------------------------
//  starts_with_whitespace_slash_slash: is this a "// comment" line
//
//  line    current line being processed
//
auto starts_with_whitespace_slash_slash(std::string const& line)
    -> bool;


//---------------------------------------------------------------------------
//  starts_with_whitespace_slash_star_and_no_star_slash: is this a "/* comment" line
//
//  line    current line being processed
//
auto starts_with_whitespace_slash_star_and_no_star_slash(std::string const& line)
    -> bool;


//---------------------------------------------------------------------------
//  starts_with_operator: returns whether the line starts with the string "operator"
//  followed by the symbols of an operator
//
//  line    current line being processed
//
auto starts_with_operator(std::string_view s)
    -> int;

//---------------------------------------------------------------------------
//  starts_with_identifier_colon: returns whether the line starts with an
//  identifier followed by one colon (not ::) (possibly preceded by an access specifier)
//
//  line    current line being processed
//
auto starts_with_identifier_colon(std::string const& line)
    -> bool;


//---------------------------------------------------------------------------
//  braces_tracker: to track brace depth
//
//  Normally we don't emit diagnostics for Cpp1 code, but we do for a
//  brace mismatch since we're relying on balanced {()} to find Cpp2 code
//
class braces_tracker
{
    //  to track preprocessor #if brace depth and brace counts
    //
    class pre_if_depth_info
    {
        int  if_net_braces   = 0;
        bool found_else      = false;
        int  else_net_braces = 0;

    public:
        auto found_open_brace() -> void;

        auto found_close_brace() -> void;

        auto found_preprocessor_else() -> void;

        //  If the "if" and "else" branches opened/closed the same net number
        //  of unbalanced braces, they were double-counted in the brace
        //  matching and to try to keep going we can apply this adjustment
        auto braces_to_ignore() -> int;
    };
    std::vector<pre_if_depth_info> preprocessor = { {} };  // sentinel
    char                           current_open_type = ' ';
    std::vector<lineno_t>          open_braces;
    std::vector<error_entry>&      errors;

public:
    braces_tracker( std::vector<error_entry>& errors );

    //  --- Brace matching functions - { and }, or ( and )

    auto found_open_brace(lineno_t lineno, char brace) -> void;

    auto found_close_brace(source_position pos, char brace) -> void;

    auto found_eof(source_position pos) const -> void;

    auto current_depth() const -> int;

    //  --- Preprocessor matching functions - #if/#else/#endif

    //  Entering an #if
    auto found_pre_if() -> void;

    //  Encountered an #else
    auto found_pre_else() -> void;

    //  Exiting an #endif
    auto found_pre_endif() -> void;
};


//---------------------------------------------------------------------------
//  starts_with_preprocessor_if_else_endif: the line starts with a preprocessor conditional
//
//  line    current line being processed
//
enum class preprocessor_conditional {
    none = 0, pre_if, pre_else, pre_endif
};
auto starts_with_preprocessor_if_else_endif(
    std::string const& line
)
    -> preprocessor_conditional;


//---------------------------------------------------------------------------
//  process_cpp_line: just enough to know what to skip over
//
//  line                current line being processed
//  in_comment          track whether we're in a comment
//  in_string_literal   track whether we're in a string literal
//
struct process_line_ret {
    bool all_comment_line;
    bool empty_line;
    bool all_rawstring_line;
};
auto process_cpp_line(
    std::string const&  line,
    bool&               in_comment,
    bool&               in_string_literal,
    bool&               in_raw_string_literal,
    std::string&        raw_string_closing_seq,
    braces_tracker&     braces,
    lineno_t            lineno
)
    -> process_line_ret;


//---------------------------------------------------------------------------
//  process_cpp2_line: to find the end of a Cpp2 definition
//      - find first of ; and {
//          - if ; we're done
//          - if { find matching }
//      - then there must be nothing else on the last line
//
//  line        current line being processed
//  in_comment  whether this line begins inside a multi-line comment
//
//  Returns:    whether additional lines should be inspected
//
auto process_cpp2_line(
    std::string const&        line,
    bool&                     in_comment,
    braces_tracker&           braces,
    lineno_t                  lineno,
    std::vector<error_entry>& errors
)
    -> bool;


//-----------------------------------------------------------------------
//
//  source: Represents a program source file
//
//-----------------------------------------------------------------------
//
class source
{
    std::vector<error_entry>& errors;
    std::vector<source_line>  lines;
    bool                      cpp1_found = false;
    bool                      cpp2_found = false;

    static const int max_line_len = 90'000;
        //  do not reduce this - I encountered an 80,556-char
        //  line in real world code during testing
    char buf[max_line_len];

public:
    //-----------------------------------------------------------------------
    //  Constructor
    //
    //  errors      error list
    //
    source(
        std::vector<error_entry>& errors_
    );


    //-----------------------------------------------------------------------
    //  has_cpp1: Returns true if this file has some Cpp1/preprocessor lines
    //            (note: import lines don't count toward Cpp1 or Cpp2)
    //
    auto has_cpp1() const -> bool;


    //-----------------------------------------------------------------------
    //  has_cpp2: Returns true if this file has some Cpp2 lines
    //            (note: import lines don't count toward Cpp1 or Cpp2)
    //
    auto has_cpp2() const -> bool;


    //-----------------------------------------------------------------------
    //  load: Read a line-by-line view of 'filename', preserving line breaks
    //
    //  filename                the source file to be loaded
    //  source                  program textual representation
    //
    auto load(
        std::string const&  filename
    )
        -> bool;


    //-----------------------------------------------------------------------
    //  get_lines: Access the source lines
    //
    auto get_lines() -> std::vector<source_line>&;

    auto get_lines() const -> std::vector<source_line> const&;

    //-----------------------------------------------------------------------
    //  debug_print
    //
    auto debug_print(std::ostream& o) const -> void;

    //  No copying
    //
    source(source const&)            = delete;
    source& operator=(source const&) = delete;
    source(source&&)                 = delete;
    source& operator=(source&&)      = delete;
};

}

#endif
