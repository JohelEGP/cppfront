
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
//  Lexer
//===========================================================================

#ifndef CPP2_LEX_H
#define CPP2_LEX_H

#include "cppfront-io-fwd.h"
#include <map>
#include <climits>
#include <deque>
#include <cstring>


namespace cpp2 {

//-----------------------------------------------------------------------
//
//  lexeme: represents the type of a token
//
//-----------------------------------------------------------------------
//

enum class lexeme : std::int8_t {
    SlashEq,
    Slash,
    LeftShiftEq,
    LeftShift,
    Spaceship,
    LessEq,
    Less,
    RightShiftEq,
    RightShift,
    GreaterEq,
    Greater,
    PlusPlus,
    PlusEq,
    Plus,
    MinusMinus,
    MinusEq,
    Arrow,
    Minus,
    LogicalOrEq,
    LogicalOr,
    PipeEq,
    Pipe,
    LogicalAndEq,
    LogicalAnd,
    MultiplyEq,
    Multiply,
    ModuloEq,
    Modulo,
    AmpersandEq,
    Ampersand,
    CaretEq,
    Caret,
    TildeEq,
    Tilde,
    EqualComparison,
    Assignment,
    NotEqualComparison,
    Not,
    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    Scope,
    Colon,
    Semicolon,
    Comma,
    Dot,
    Ellipsis,
    QuestionMark,
    At,
    Dollar,
    FloatLiteral,
    BinaryLiteral,
    DecimalLiteral,
    HexadecimalLiteral,
    StringLiteral,
    CharacterLiteral,
    UserDefinedLiteralSuffix,
    Keyword,
    Cpp1MultiKeyword,
    Cpp2FixedType,
    Identifier,
    None = 127
};

auto is_literal(lexeme l) -> bool;

auto close_paren_type(lexeme l)
    -> lexeme;


template<typename T>
    requires std::is_same_v<T, std::string>
auto _as(lexeme l)
    -> std::string;;


auto is_operator(lexeme l)
    -> bool;


//-----------------------------------------------------------------------
//
//  token: represents a single token
//
//     Note: by reference, thge test into the program's source lines
//
//-----------------------------------------------------------------------
//
class token
{
public:
    token(
        char const*     start,
        auto            count,
        source_position pos,
        lexeme          type
    );

    token(
        char const*     sz,
        source_position pos,
        lexeme          type
    );

    auto as_string_view() const
        -> std::string_view;

    operator std::string_view() const;

    auto operator== (token const& t) const
        -> bool;

    auto operator== (std::string_view s) const
        -> bool;

    auto to_string() const
        -> std::string;

    friend auto operator<< (auto& o, token const& t)
        -> auto&;

    auto position_col_shift( colno_t offset )
        -> void;

    auto position() const -> source_position;

    auto length  () const -> int;

    auto type    () const -> lexeme;

    auto set_type(lexeme l) -> void;

    auto visit(auto& v, int depth) const
        -> void;

    auto remove_prefix_if(std::string_view prefix);

private:
    std::string_view sv;
    source_position  pos;
    lexeme           lex_type;
};

static_assert (CHAR_BIT == 8);


auto labelized_position(token const* t)
    -> std::string;


//-----------------------------------------------------------------------
//
//  A StringLiteral could include captures
//
auto expand_string_literal(
    std::string_view          text,
    std::vector<error_entry>& errors,
    source_position           src_pos
)
    -> std::string;

auto expand_raw_string_literal(
    const std::string&           opening_seq,
    const std::string&           closing_seq,
    string_parts::adds_sequences closing_strategy,
    std::string_view             text,
    std::vector<error_entry>&    errors,
    source_position src_pos
)
    -> string_parts;

//-----------------------------------------------------------------------
//  lex: Tokenize a single line while maintaining inter-line state
//
//  mutable_line            the line to be tokenized
//  lineno                  the current line number
//  in_comment              are we currently in a comment
//  current_comment         the current partial comment
//  current_comment_start   the current comment's start position
//  tokens                  the token list to add to
//  comments                the comment token list to add to
//  errors                  the error message list to use for reporting problems
//  raw_string_multiline    the current optional raw_string state
//

auto lex_line(
    std::string&               mutable_line,
    int const                  lineno,
    bool&                      in_comment,
    std::string&               current_comment,
    source_position&           current_comment_start,
    std::vector<token>&        tokens,
    std::vector<comment>&      comments,
    std::vector<error_entry>&  errors,
    std::optional<raw_string>& raw_string_multiline
)
    -> bool;


//-----------------------------------------------------------------------
//
//  tokens: a map of the tokens of a source file
//
//-----------------------------------------------------------------------
//

class tokens
{
    std::vector<error_entry>& errors;

    //  All non-comment source tokens go here, which will be parsed in the parser
    std::map<lineno_t, std::vector<token>> grammar_map;

    //  All comment source tokens go here, which are applied in the lexer
    //
    //  We could put all the tokens in the same map, but that would mean the
    //  parsing logic would have to remember to skip comments everywhere...
    //  simpler to keep comments separate, at the smaller cost of traversing
    //  a second token stream when lowering to Cpp1 to re-interleave comments
    std::vector<comment> comments;

    //  A stable place to store additional tokens that are synthesized later
    std::deque<token> generated_tokens;

public:
    //-----------------------------------------------------------------------
    //  Constructor
    //
    //  errors      error list
    //
    tokens(
        std::vector<error_entry>& errors_
    );


    //-----------------------------------------------------------------------
    //  lex: Tokenize the Cpp2 lines
    //
    //  lines           tagged source lines
    //  is_generated    is this generated code
    //
    auto lex(
        std::vector<source_line>& lines,
        bool                      is_generated = false
    )
        -> void;


    //-----------------------------------------------------------------------
    //  get_map: Access the token map
    //
    auto get_map() const
        -> auto const&;


    //-----------------------------------------------------------------------
    //  get_comments: Access the comment list
    //
    auto get_comments() const
        -> auto const&;


    //-----------------------------------------------------------------------
    //  get_generated: Access the generated tokens
    //
    auto get_generated()
        -> auto&;


    //-----------------------------------------------------------------------
    //  num_unprinted_comments: The number of not-yet-printed comments
    //
    auto num_unprinted_comments()
        -> int;

    //-----------------------------------------------------------------------
    //  debug_print
    //
    auto debug_print(std::ostream& o) const
        -> void;

};

}

#endif
