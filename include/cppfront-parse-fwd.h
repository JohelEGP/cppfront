
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
//  Parser
//===========================================================================

#ifndef CPP2_PARSE_H
#define CPP2_PARSE_H

#include "cppfront-lex-fwd.h"
#include <memory>
#include <variant>
#include <iostream>


namespace cpp2 {

//-----------------------------------------------------------------------
//
//  Parse tree node types
//
//-----------------------------------------------------------------------
//


struct expression_list_node;
struct id_expression_node;
struct declaration_node;
struct inspect_expression_node;
struct literal_node;
struct template_argument;


struct primary_expression_node
{
    enum active { empty=0, identifier, expression_list, id_expression, declaration, inspect, literal };
    std::variant<
        std::monostate,
        token const*,
        std::unique_ptr<expression_list_node>,
        std::unique_ptr<id_expression_node>,
        std::unique_ptr<declaration_node>,
        std::unique_ptr<inspect_expression_node>,
        std::unique_ptr<literal_node>
    > expr;


    //  API
    //
    auto is_fold_expression() const
        -> bool;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto is_literal() const
        -> bool;

    auto template_arguments() const -> std::vector<template_argument> const&;

    auto get_token() const -> token const*;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const -> source_position;
    auto visit(auto& v, int depth) -> void;
};


struct literal_node {
    token const* literal             = {};
    token const* user_defined_suffix = {};

    //  API
    //
    auto get_token() const
        -> token const*;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth) -> void;
};


struct postfix_expression_node;

struct prefix_expression_node
{
    std::vector<token const*> ops;
    std::unique_ptr<postfix_expression_node> expr;

    //  API
    //
    auto is_fold_expression() const
        -> bool;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto get_postfix_expression_node() const
        -> postfix_expression_node *;

    auto is_literal() const
        -> bool;

    auto is_result_a_temporary_variable() const -> bool;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const -> source_position;
    auto visit(auto& v, int depth) -> void;
};


struct expression_node;


template<
    String   Name,
    typename Term
>
struct binary_expression_node
{
    std::unique_ptr<Term>  expr;
    expression_node const* my_expression = {};

    binary_expression_node();

    struct term
    {
        token const* op;
        std::unique_ptr<Term> expr;
    };
    std::vector<term> terms;


    //  API
    //
    auto is_fold_expression() const
        -> bool;

    auto lhs_is_id_expression() const
        -> bool;

    auto is_standalone_expression() const
        -> bool;

    auto terms_size() const
        -> int;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto is_literal() const
        -> bool;

    //  Get left-hand postfix-expression
    auto get_postfix_expression_node() const
        -> postfix_expression_node *;

    //  Get first right-hand postfix-expression, if there is one
    auto get_second_postfix_expression_node() const
        -> postfix_expression_node *;

    //  "Simple" means binary (size>0) and not chained (size<2)
    struct get_lhs_rhs_if_simple_binary_expression_with_ret {
        postfix_expression_node* lhs;
        Term*                    rhs;
    };
    auto get_lhs_rhs_if_simple_binary_expression_with(lexeme op) const
        -> get_lhs_rhs_if_simple_binary_expression_with_ret;

    auto is_result_a_temporary_variable() const -> bool;

    auto to_string() const
        -> std::string;


    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct is_as_expression_node;

using multiplicative_expression_node = binary_expression_node< "multiplicative" , is_as_expression_node          >;
using additive_expression_node       = binary_expression_node< "additive"       , multiplicative_expression_node >;
using shift_expression_node          = binary_expression_node< "shift"          , additive_expression_node       >;
using compare_expression_node        = binary_expression_node< "compare"        , shift_expression_node          >;
using relational_expression_node     = binary_expression_node< "relational"     , compare_expression_node        >;
using equality_expression_node       = binary_expression_node< "equality"       , relational_expression_node     >;
using bit_and_expression_node        = binary_expression_node< "bit-and"        , equality_expression_node       >;
using bit_xor_expression_node        = binary_expression_node< "bit-xor"        , bit_and_expression_node        >;
using bit_or_expression_node         = binary_expression_node< "bit-or"         , bit_xor_expression_node        >;
using logical_and_expression_node    = binary_expression_node< "logical-and"    , bit_or_expression_node         >;
using logical_or_expression_node     = binary_expression_node< "logical-or"     , logical_and_expression_node    >;
using assignment_expression_node     = binary_expression_node< "assignment"     , logical_or_expression_node     >;


struct assignment_expression_lhs_rhs {
    postfix_expression_node*    lhs;
    logical_or_expression_node* rhs;
};


struct expression_statement_node;

struct expression_node
{
    static inline std::vector<expression_node*> current_expressions = {};

    std::unique_ptr<assignment_expression_node> expr;
    int num_subexpressions = 0;
    expression_statement_node const* my_statement = {};

    expression_node();

    // API
    //
    auto is_fold_expression() const
        -> bool;

    auto is_standalone_expression() const
        -> bool;

    auto subexpression_count() const
        -> int;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto is_literal() const
        -> bool;

    auto get_lhs_rhs_if_simple_assignment() const
        -> assignment_expression_lhs_rhs;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const -> source_position;

    auto visit(auto& v, int depth) -> void;
};


enum class passing_style { in=0, copy, inout, out, move, forward, invalid };
auto to_passing_style(token const& t) -> passing_style;
auto to_string_view(passing_style pass) -> std::string_view;


struct expression_list_node
{
    token const* open_paren  = {};
    token const* close_paren = {};
    bool inside_initializer  = false;

    struct term {
        passing_style                    pass = {};
        std::unique_ptr<expression_node> expr;

        auto visit(auto& v, int depth) -> void;
    };
    std::vector< term > expressions;


    //  API
    //
    auto is_fold_expression() const
        -> bool;


    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct expression_statement_node
{
    static inline std::vector<expression_statement_node*> current_expression_statements = {};

    std::unique_ptr<expression_node> expr;
    bool has_semicolon = false;

    //  API
    //
    auto subexpression_count() const
        -> int;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct capture {
    postfix_expression_node* capture_expr;
    std::string              cap_sym = {};
    std::string              str = {};
    std::string              str_suppressed_move = {};
    auto operator==(postfix_expression_node* p);
};

struct capture_group {
    std::vector<capture> members;

    auto add(postfix_expression_node* p)
        -> void;

    auto remove(postfix_expression_node* p)
        -> void;

    ~capture_group();
};


struct postfix_expression_node
{
    std::unique_ptr<primary_expression_node> expr;

    struct term
    {
        token const* op;

        //  This is used if *op is . - can be null
        std::unique_ptr<id_expression_node> id_expr = {};

        //  These are used if *op is [ or ( - can be null
        std::unique_ptr<expression_list_node> expr_list = {};
        token const* op_close = {};
    };
    std::vector<term> ops;
    capture_group* cap_grp = {};

    ~postfix_expression_node();

    //  API
    //
    auto is_fold_expression() const
        -> bool;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto is_literal() const
        -> bool;

    auto get_first_token_ignoring_this() const
        -> token const*;

    auto is_result_a_temporary_variable() const -> bool;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const -> source_position;

    auto visit(auto& v, int depth) -> void;
};


struct type_id_node;
struct template_args_tag { };

struct template_argument
{
    enum active { empty=0, expression, type_id };
    source_position comma;
    std::variant<
        std::monostate,
        std::unique_ptr<expression_node>,
        std::unique_ptr<type_id_node>
    > arg;

    auto to_string() const
        -> std::string;
};

// Used by functions that must return a reference to an empty arg list
inline std::vector<template_argument> const no_template_args;

struct unqualified_id_node
{
    token const* identifier      = {};  // required

    // These are used only if it's a template-id
    source_position open_angle  = {};
    source_position close_angle = {};

    std::vector<template_argument> template_args;

    auto template_arguments() const
        -> std::vector<template_argument> const&;

    auto get_token() const
        -> token const*;

    auto to_string() const
        -> std::string;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct qualified_id_node
{
    struct term {
        token const* scope_op;
        std::unique_ptr<unqualified_id_node> id = {};

        term( token const* o );
    };
    std::vector<term> ids;

    auto template_arguments() const
        -> std::vector<template_argument> const&;

    auto get_token() const
        -> token const*;

    auto to_string() const
        -> std::string;

    auto get_first_token() const
        -> token const*;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct type_id_node
{
    source_position pos;

    std::vector<token const*> pc_qualifiers;
    token const* address_of                 = {};
    token const* dereference_of             = {};
    int dereference_cnt                     = {};
    token const* suspicious_initialization  = {};

    enum active { empty=0, qualified, unqualified, keyword };
    std::variant<
        std::monostate,
        std::unique_ptr<qualified_id_node>,
        std::unique_ptr<unqualified_id_node>,
        token const*
    > id;

    auto is_wildcard() const
        -> bool;

    auto is_pointer_qualified() const
        -> bool;

    auto is_concept() const
        -> bool;

    auto template_arguments() const
        -> std::vector<template_argument> const&;

    auto to_string() const
        -> std::string;

    auto get_token() const
        -> token const*;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct is_as_expression_node
{
    std::unique_ptr<prefix_expression_node> expr;

    struct term
    {
        token const* op = {};

        //  This is used if *op is a type - can be null
        std::unique_ptr<type_id_node> type = {};

        //  This is used if *op is an expression - can be null
        std::unique_ptr<expression_node> expr = {};
    };
    std::vector<term> ops;


    //  API
    //
    auto is_fold_expression() const
        -> bool;

    auto is_identifier() const
        -> bool;

    auto is_id_expression() const
        -> bool;

    auto is_expression_list() const
        -> bool;

    auto get_expression_list() const
        -> expression_list_node const*;

    auto is_literal() const
        -> bool;

    auto get_postfix_expression_node() const
        -> postfix_expression_node *;

    auto is_result_a_temporary_variable() const -> bool;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct id_expression_node
{
    source_position pos;

    enum active { empty=0, qualified, unqualified };
    std::variant<
        std::monostate,
        std::unique_ptr<qualified_id_node>,
        std::unique_ptr<unqualified_id_node>
    > id;

    auto template_arguments() const
        -> std::vector<template_argument> const&;

    auto is_fold_expression() const
        -> bool;

    auto is_empty() const
        -> bool;

    auto is_qualified() const
        -> bool;

    auto is_unqualified() const
        -> bool;

    auto get_token() const
        -> token const*;

    auto to_string() const
        -> std::string;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct statement_node;

struct compound_statement_node
{
    source_position open_brace;
    source_position close_brace;
    std::vector<std::unique_ptr<statement_node>> statements;

    colno_t body_indent = 0;

    compound_statement_node(source_position o = source_position{});

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth) -> void;
};


struct selection_statement_node
{
    bool                                        is_constexpr = false;
    token const*                                identifier   = {};
    source_position                             else_pos;
    std::unique_ptr<logical_or_expression_node> expression;
    std::unique_ptr<compound_statement_node>    true_branch;
    std::unique_ptr<compound_statement_node>    false_branch;
    bool                                        has_source_false_branch = false;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct parameter_declaration_node;

struct iteration_statement_node
{
    token const*                                label      = {};
    token const*                                identifier = {};
    std::unique_ptr<assignment_expression_node> next_expression;    // if used, else null
    std::unique_ptr<logical_or_expression_node> condition;          // used for "do" and "while", else null
    std::unique_ptr<compound_statement_node>    statements;         // used for "do" and "while", else null
    std::unique_ptr<expression_node>            range;              // used for "for", else null
    std::unique_ptr<parameter_declaration_node> parameter;          // used for "for", else null
    std::unique_ptr<statement_node>             body;               // used for "for", else null
    bool                                        for_with_in = false;// used for "for," says whether loop variable is 'in'

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct return_statement_node
{
    token const*                     identifier = {};
    std::unique_ptr<expression_node> expression;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct alternative_node
{
    std::unique_ptr<unqualified_id_node> name;
    token const*                         is_as_keyword = {};

    //  One of these will be used
    std::unique_ptr<type_id_node>            type_id;
    std::unique_ptr<postfix_expression_node> value;

    source_position                      equal_sign;
    std::unique_ptr<statement_node>      statement;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct inspect_expression_node
{
    bool                                     is_constexpr = false;
    token const*                             identifier   = {};
    std::unique_ptr<expression_node>         expression;
    std::unique_ptr<type_id_node>            result_type;
    source_position                          open_brace;
    source_position                          close_brace;

    std::vector<std::unique_ptr<alternative_node>> alternatives;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct contract_node
{
    //  Declared first, because it should outlive any owned
    //  postfix_expressions that could refer to it
    capture_group captures;

    source_position                             open_bracket;
    token const*                                kind = {};
    std::unique_ptr<id_expression_node>         group;
    std::unique_ptr<logical_or_expression_node> condition;
    token const*                                message = {};

    contract_node( source_position pos );

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct jump_statement_node
{
    token const* keyword;
    token const* label;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct using_statement_node
{
    token const*                        keyword = {};
    bool                                for_namespace = false;
    std::unique_ptr<id_expression_node> id;

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct parameter_declaration_list_node;

struct statement_node
{
    std::unique_ptr<parameter_declaration_list_node> parameters;
    compound_statement_node* compound_parent = nullptr;

    statement_node(compound_statement_node* compound_parent_ = nullptr);

    enum active { expression=0, compound, selection, declaration, return_, iteration, using_, contract, inspect, jump };
    std::variant<
        std::unique_ptr<expression_statement_node>,
        std::unique_ptr<compound_statement_node>,
        std::unique_ptr<selection_statement_node>,
        std::unique_ptr<declaration_node>,
        std::unique_ptr<return_statement_node>,
        std::unique_ptr<iteration_statement_node>,
        std::unique_ptr<using_statement_node>,
        std::unique_ptr<contract_node>,
        std::unique_ptr<inspect_expression_node>,
        std::unique_ptr<jump_statement_node>
    > statement;

    bool emitted = false;               // a note field that's used during lowering to Cpp1

    bool marked_for_removal = false;    // for use during metafunctions which may replace members

    //  API
    //
    auto is_expression () const -> bool;
    auto is_compound   () const -> bool;
    auto is_selection  () const -> bool;
    auto is_declaration() const -> bool;
    auto is_return     () const -> bool;
    auto is_iteration  () const -> bool;
    auto is_using      () const -> bool;
    auto is_contract   () const -> bool;
    auto is_inspect    () const -> bool;
    auto is_jump       () const -> bool;

    template<typename Node>
    auto get_if()
        -> Node*;

    template<typename Node>
    auto get_if() const
        -> Node const*;

    auto get_lhs_rhs_if_simple_assignment() const
        -> assignment_expression_lhs_rhs;

    auto to_string() const
        -> std::string;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct parameter_declaration_node
{
    source_position pos = {};
    passing_style pass  = passing_style::in;
    int ordinal = 1;

    enum class modifier { none=0, implicit, virtual_, override_, final_ };
    modifier mod = modifier::none;

    std::unique_ptr<declaration_node> declaration;

    //  API
    //
    auto has_name() const
        -> bool;

    auto name() const
        -> token const*;

    auto has_name(std::string_view) const
        -> bool;

    auto direction() const
        -> passing_style;

    auto is_implicit() const
        -> bool;

    auto is_virtual() const
        -> bool;

    auto make_virtual()
        -> void;

    auto is_override() const
        -> bool;

    auto is_final() const
        -> bool;

    auto is_polymorphic() const
        -> bool;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct parameter_declaration_list_node
{
    token const* open_paren  = {};
    token const* close_paren = {};

    std::vector<std::unique_ptr<parameter_declaration_node>> parameters;

    //  API
    //
    auto ssize() const -> auto;

    auto operator[](int i)
        -> parameter_declaration_node*;

    auto operator[](int i) const
        -> parameter_declaration_node const*;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct function_returns_tag { };

struct function_type_node
{
    declaration_node* my_decl;

    std::unique_ptr<parameter_declaration_list_node> parameters;
    bool throws = false;

    struct single_type_id {
        std::unique_ptr<type_id_node> type;
        passing_style pass = passing_style::move;
    };

    enum active { empty = 0, id, list };
    std::variant<
        std::monostate,
        single_type_id,
        std::unique_ptr<parameter_declaration_list_node>
    > returns;

    std::vector<std::unique_ptr<contract_node>> contracts;

    function_type_node(declaration_node* decl);

    //  API
    //
    auto is_function_with_this() const
        -> bool;

    auto is_virtual_function() const
        -> bool;

    auto make_function_virtual()
        -> bool;

    auto is_defaultable() const
        -> bool;

    auto is_constructor() const
        -> bool;

    auto is_default_constructor() const
        -> bool;

    auto is_move() const
        -> bool;

    auto is_swap() const
        -> bool;

    auto is_constructor_with_that() const
        -> bool;

    auto is_constructor_with_in_that() const
        -> bool;

    auto is_constructor_with_move_that() const
        -> bool;

    auto is_comparison() const
        -> bool;

    auto is_compound_assignment() const
        -> bool;

    auto is_assignment() const
        -> bool;

    auto is_assignment_with_that() const
        -> bool;

    auto is_assignment_with_in_that() const
        -> bool;

    auto is_assignment_with_move_that() const
        -> bool;

    auto is_destructor() const
        -> bool;

    auto is_metafunction() const
        -> bool;

    auto has_declared_return_type() const
        -> bool;

    auto unnamed_return_type_to_string() const
        -> std::string;

    auto has_bool_return_type() const
        -> bool;

    auto has_non_void_return_type() const
        -> bool;

    auto parameter_count() const
        -> int;

    auto index_of_parameter_named(std::string_view s) const
        -> int;

    auto has_parameter_named(std::string_view s) const
        -> bool;

    auto has_parameter_with_name_and_pass(
        std::string_view s,
        passing_style    pass
    ) const
        -> bool;

    auto nth_parameter_type_name(int n) const
        -> std::string;

    auto has_in_parameter_named(std::string_view s) const
        -> bool;

    auto has_out_parameter_named(std::string_view s) const
        -> bool;

    auto has_move_parameter_named(std::string_view s) const
        -> bool;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct type_node
{
    token const* type;
    bool         final = false;

    type_node(
        token const* t,
        bool         final_ = false
    );

    //  API
    //
    auto is_final() const
        -> bool;

    auto make_final()
        -> void;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct namespace_node
{
    token const* namespace_;

    namespace_node(token const* ns);

    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct alias_node
{
    token const* type = {};
    std::unique_ptr<type_id_node> type_id;   // for objects

    enum active : std::uint8_t { a_type, a_namespace, an_object };
    std::variant<
        std::unique_ptr<type_id_node>,
        std::unique_ptr<id_expression_node>,
        std::unique_ptr<expression_node>
    > initializer;

    alias_node( token const* t );

    //  API
    //
    auto is_type_alias     () const -> bool;
    auto is_namespace_alias() const -> bool;
    auto is_object_alias   () const -> bool;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


enum class accessibility { default_ = 0, public_, protected_, private_ };

auto to_string(accessibility a)
    -> std::string;


struct declaration_identifier_tag { };

struct declaration_node
{
    //  The capture_group is declared first, because it should outlive
    //  any owned postfix_expressions that could refer to it
    capture_group                        captures;
    source_position                      pos;
    bool                                 is_variadic = false;
    bool                                 is_constexpr = false;
    bool                                 terse_no_equals = false;
    std::unique_ptr<unqualified_id_node> identifier;
    accessibility                        access = accessibility::default_;

    enum active : std::uint8_t { a_function, an_object, a_type, a_namespace, an_alias };
    std::variant<
        std::unique_ptr<function_type_node>,
        std::unique_ptr<type_id_node>,
        std::unique_ptr<type_node>,
        std::unique_ptr<namespace_node>,
        std::unique_ptr<alias_node>
    > type;

    std::vector<std::unique_ptr<id_expression_node>> metafunctions;
    std::unique_ptr<parameter_declaration_list_node> template_parameters;
    source_position                                  requires_pos = {};
    std::unique_ptr<logical_or_expression_node>      requires_clause_expression;

    source_position                 equal_sign = {};
    std::unique_ptr<statement_node> initializer;

    declaration_node*               parent_declaration = {};
    statement_node*                 my_statement = {};

    //  Attributes currently configurable only via metafunction API,
    //  not directly in the base language grammar
    bool member_function_generation = true;

    //  Cache some context
    bool is_template_parameter = false;
    bool is_parameter          = false;

    //  Constructor
    //
    declaration_node(declaration_node* parent);

    //  API
    //
    auto type_member_mark_for_removal()
        -> bool;

    auto type_remove_marked_members()
        -> void;

    auto type_remove_all_members()
        -> void;

    auto type_disable_member_function_generation()
        -> void;

    auto object_type() const
        -> std::string;

    auto object_initializer() const
        -> std::string;

    auto get_parent() const
        -> declaration_node*;

    auto is_public() const
        -> bool;

    auto is_protected() const
        -> bool;

    auto is_private() const
        -> bool;

    auto is_default_access() const
        -> bool;

private:
    auto set_access(accessibility a)
        -> bool;

public:
    auto make_public()
        -> bool;

    auto make_protected()
        -> bool;

    auto make_private()
        -> bool;

    auto has_name() const
        -> bool;

    auto name() const
        -> token const*;

    auto has_name(std::string_view s) const
        -> bool;

    auto has_initializer() const
        -> bool;

    auto parameter_count() const
        -> int;

    auto index_of_parameter_named(std::string_view s) const
        -> int;

    auto has_parameter_named(std::string_view s) const
        -> bool;

    auto has_in_parameter_named(std::string_view s) const
        -> bool;

    auto has_out_parameter_named(std::string_view s) const
        -> bool;

    auto has_move_parameter_named(std::string_view s) const
        -> bool;

    auto nth_parameter_type_name(int n) const
        -> std::string;

    auto is_global   () const -> bool;

    auto is_function () const -> bool;
    auto is_object   () const -> bool;
    auto is_base_object() const -> bool;
    auto is_member_object() const -> bool;
    auto is_concept  () const -> bool;
    auto is_type     () const -> bool;
    auto is_namespace() const -> bool;
    auto is_alias() const -> bool;

    auto is_type_alias     () const -> bool;
    auto is_namespace_alias() const -> bool;
    auto is_object_alias   () const -> bool;

    auto is_function_expression () const -> bool;

    auto is_polymorphic() const // has base types or virtual functions
        -> bool;

    auto parent_is_function   () const -> bool;
    auto parent_is_object     () const -> bool;
    auto parent_is_type       () const -> bool;
    auto parent_is_namespace  () const -> bool;
    auto parent_is_alias      () const -> bool;
    auto parent_is_polymorphic() const -> bool;

    enum which {
        functions = 1,
        objects   = 2,
        types     = 4,
        aliases   = 8,
        all       = functions|objects|types|aliases
    };

private:
    //  This helper is a const function that delivers pointers
    //  to non-const... because this is the best way I can
    //  think of right now to write the following two get_
    //  functions (without duplicating their bodies, and
    //  without resorting to const_casts)
    auto gather_type_scope_declarations(which w) const
        -> std::vector<declaration_node*>;

public:
    auto get_type_scope_declarations(which w = all)
        -> std::vector<declaration_node*>;

    auto get_type_scope_declarations(which w = all) const
        -> std::vector<declaration_node const*>;


    auto add_type_member( std::unique_ptr<statement_node> statement )
        -> bool;


    auto get_decl_if_type_scope_object_name_before_a_base_type( std::string_view s ) const
        -> declaration_node const*;


    auto get_initializer_statements() const
        -> std::vector<statement_node*>;

    auto is_function_with_this() const
        -> bool;

    auto is_virtual_function() const
        -> bool;

    auto is_type_final() const
        -> bool;

    auto make_type_final()
        -> bool;

    auto make_function_virtual()
        -> bool;

    auto is_defaultable_function() const
        -> bool;

    auto is_constructor() const
        -> bool;

    auto is_default_constructor() const
        -> bool;

    auto is_move() const
        -> bool;

    auto is_swap() const
        -> bool;

    auto is_constructor_with_that() const
        -> bool;

    auto is_constructor_with_in_that() const
        -> bool;

    auto is_constructor_with_move_that() const
        -> bool;

    auto is_comparison() const
        -> bool;

    auto is_compound_assignment() const
        -> bool;

    auto is_assignment() const
        -> bool;

    auto is_assignment_with_that() const
        -> bool;

    auto is_assignment_with_in_that() const
        -> bool;

    auto is_assignment_with_move_that() const
        -> bool;

    struct declared_value_set_funcs {
        declaration_node const*  out_this_in_that     = {};
        declaration_node const*  out_this_move_that   = {};
        declaration_node const*  inout_this_in_that   = {};
        declaration_node const*  inout_this_move_that = {};
        std::vector<std::string> assignments_from     = {};
    };

    auto find_declared_value_set_functions() const
        -> declared_value_set_funcs;

    auto find_parent_declared_value_set_functions() const
        -> declared_value_set_funcs;


    auto is_destructor() const
        -> bool;

    auto has_declared_return_type() const
        -> bool;

    auto unnamed_return_type_to_string() const
        -> std::string;

    auto has_bool_return_type() const
        -> bool;

    auto has_non_void_return_type() const
        -> bool;

    auto has_parameter_with_name_and_pass(
        std::string_view s,
        passing_style    pass
    ) const
        -> bool;

    auto is_metafunction() const
        -> bool;

    auto is_binary_comparison_function() const
        -> bool;

    auto is_const() const
        -> bool;

    auto has_wildcard_type() const
        -> bool;

    auto get_object_type() const
        -> type_id_node const*;

    //  Internals
    //
    auto position() const
        -> source_position;

    auto visit(auto& v, int depth)
        -> void;
};


struct next_expression_tag { };


struct translation_unit_node
{
    std::vector< std::unique_ptr<declaration_node> > declarations;

    auto position() const -> source_position;

    auto visit(auto& v, int depth) -> void;
};


//-----------------------------------------------------------------------
//
//  pretty_print_visualize: pretty-prints Cpp2 ASTs
//
//-----------------------------------------------------------------------
//
auto pretty_print_visualize(token const& n, int indent)
    -> std::string;
auto pretty_print_visualize(primary_expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(literal_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(prefix_expression_node const& n, int indent)
    -> std::string;
template<
    String   Name,
    typename Term
>
auto pretty_print_visualize(binary_expression_node<Name,Term> const& n, int indent)
    -> std::string;
auto pretty_print_visualize(expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(expression_list_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(expression_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(postfix_expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(unqualified_id_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(qualified_id_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(type_id_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(is_as_expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(id_expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(compound_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(selection_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(iteration_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(return_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(alternative_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(inspect_expression_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(contract_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(jump_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(using_statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(statement_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(parameter_declaration_node const& n, int indent, bool is_template_param = false)
    -> std::string;
auto pretty_print_visualize(parameter_declaration_list_node const& n, int indent, bool is_template_param_list = false)
    -> std::string;
auto pretty_print_visualize(function_type_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(type_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(namespace_node const& n, int indent)
    -> std::string;
auto pretty_print_visualize(declaration_node const& n, int indent, bool include_metafunctions_list = false)
    -> std::string;



//-----------------------------------------------------------------------
//  pre: Get an indentation prefix
//

auto pre(int indent)
    -> std::string_view;


//-----------------------------------------------------------------------
//  try_pretty_print_visualize
//
//  Helper to emit whatever is in a variant where each
//  alternative is a smart pointer
//
template <int I>
auto try_pretty_print_visualize(
    auto&     v,
    auto&&... more
)
    -> std::string;

//-----------------------------------------------------------------------
//
//  parser: parses a section of Cpp2 code
//
//-----------------------------------------------------------------------
//
class parser
{
    std::vector<error_entry>& errors;

    std::unique_ptr<translation_unit_node> parse_tree = {};

    //  Keep a stack of current capture groups (contracts/decls still being parsed)
    std::vector<capture_group*> current_capture_groups = {};

    struct capture_groups_stack_guard
    {
        parser* pars;

        capture_groups_stack_guard(parser* p, capture_group* cg);

        ~capture_groups_stack_guard();
    };

    //  Keep a stack of currently active declarations (still being parsed)
    std::vector<declaration_node*> current_declarations = { nullptr };

    struct current_declarations_stack_guard
    {
        parser* pars;

        current_declarations_stack_guard(parser* p, declaration_node* decl);

        ~current_declarations_stack_guard();
    };

    std::vector<token> const* tokens = {};
    std::deque<token>* generated_tokens = {};
    int pos = 0;
    std::string parse_kind = {};

    //  Keep track of the function bodies' locations - used to emit comments
    //  in the right pass (decide whether it's a comment that belongs with
    //  the declaration or is part of the definition)
    struct function_body_extent {
        lineno_t first;
        lineno_t last;
        auto operator<=>(function_body_extent const&) const = default;
        auto operator<=>(int i) const;

        function_body_extent( lineno_t f, lineno_t l );
    };
    mutable std::vector<function_body_extent> function_body_extents;
    mutable bool                              is_function_body_extents_sorted = false;

public:
    auto is_within_function_body(source_position p) const;


public:
    //-----------------------------------------------------------------------
    //  Constructors - the copy constructor constructs a new instance with
    //                 the same errors reference but otherwise a clean slate
    //
    //  errors      error list
    //
    parser( std::vector<error_entry>& errors_ );

    parser( parser const& that );


    //-----------------------------------------------------------------------
    //  parse
    //
    //  tokens              input tokens for this section of Cpp2 source code
    //  generated_tokens    a shared place to store generated tokens
    //
    //  Each call parses this section's worth of tokens and adds the
    //  result to the stored parse tree. Call this repeatedly for the Cpp2
    //  sections in a TU to build the whole TU's parse tree
    //
    auto parse(
        std::vector<token> const& tokens_,
        std::deque<token>&        generated_tokens_
    )
        -> bool;


    //-----------------------------------------------------------------------
    //  parse_one_statement
    //
    //  tokens              input tokens for this section of Cpp2 source code
    //  generated_tokens    a shared place to store generated tokens
    //
    //  Each call parses one statement and returns its parse tree.
    //
    auto parse_one_declaration(
        std::vector<token> const& tokens_,
        std::deque<token>&        generated_tokens_
    )
        -> std::unique_ptr<statement_node>;


    //-----------------------------------------------------------------------
    //  Get a set of pointers to just the declarations in the given token map section
    //
    auto get_parse_tree_declarations_in_range(std::vector<token> const& token_range) const
        -> std::vector< declaration_node const* >;


    //-----------------------------------------------------------------------
    //  visit
    //
    auto visit(auto& v) -> void;

private:
    //-----------------------------------------------------------------------
    //  Error reporting: Fed into the supplied this->errors object
    //
    //  msg                 message to be printed
    //
    //  include_curr_token  in this file (during parsing), we normally want
    //                      to show the current token as the unexpected text
    //                      we encountered, but some sema rules are applied
    //                      early during parsing and for those it doesn't
    //                      make sense to show the next token (e.g., when
    //                      we detect and reject a "std::move" qualified-id,
    //                      it's not relevant to add "at LeftParen: ("
    //                      just because ( happens to be the next token)
    //
    auto error(
        char const*     msg,
        bool            include_curr_token = true,
        source_position err_pos            = {},
        bool            fallback           = false
    ) const
        -> void;

    auto error(
        std::string const& msg,
        bool               include_curr_token = true,
        source_position    err_pos            = {},
        bool               fallback           = false
    ) const
        -> void;

    bool has_error();


    //-----------------------------------------------------------------------
    //  Token navigation: Only these functions should access this->token_
    //
    auto curr() const
        -> token const&;

    auto peek(int num) const
        -> token const*;

    auto done() const
        -> bool;

    auto next(int num = 1)
        -> void;


    //-----------------------------------------------------------------------
    //  Parsers for unary expressions
    //

    //G primary-expression:
    //G     inspect-expression
    //G     id-expression
    //G     literal
    //G     '(' expression-list ')'
    //GT     '{' expression-list '}'
    //G     unnamed-declaration
    //G
    auto primary_expression()
        -> std::unique_ptr<primary_expression_node>;


    //G postfix-expression:
    //G     primary-expression
    //G     postfix-expression postfix-operator     [Note: without whitespace before the operator]
    //G     postfix-expression '[' expression-list? ']'
    //G     postfix-expression '(' expression-list? ')'
    //G     postfix-expression '.' id-expression
    //G
    auto postfix_expression()
        -> std::unique_ptr<postfix_expression_node>;


    //G prefix-expression:
    //G     postfix-expression
    //G     prefix-operator prefix-expression
    //GTODO     await-expression
    //GTODO     'sizeof' '(' type-id ')'
    //GTODO     'sizeof' '...' ( identifier ')'
    //GTODO     'alignof' '(' type-id ')'
    //GTODO     throws-expression
    //G
    auto prefix_expression()
        -> std::unique_ptr<prefix_expression_node>;


    //-----------------------------------------------------------------------
    //  Parsers for binary expressions
    //

    //  The general /*binary*/-expression:
    //     /*term*/-expression { { /* operators at this precedence level */ } /*term*/-expression }*
    //
    template<
        typename Binary,
        typename ValidateOp,
        typename TermFunc
    >
    auto binary_expression(
        ValidateOp validate_op,
        TermFunc   term
    )
        -> std::unique_ptr<Binary>;

    //G multiplicative-expression:
    //G     is-as-expression
    //G     multiplicative-expression '*' is-as-expression
    //G     multiplicative-expression '/' is-as-expression
    //G     multiplicative-expression '%' is-as-expression
    //G
    auto multiplicative_expression()
        -> auto;

    //G additive-expression:
    //G     multiplicative-expression
    //G     additive-expression '+' multiplicative-expression
    //G     additive-expression '-' multiplicative-expression
    //G
    auto additive_expression()
        -> auto;

    //G shift-expression:
    //G     additive-expression
    //G     shift-expression '<<' additive-expression
    //G     shift-expression '>>' additive-expression
    //G
    auto shift_expression(bool allow_angle_operators = true)
        -> auto;

    //G compare-expression:
    //G     shift-expression
    //G     compare-expression '<=>' shift-expression
    //G
    auto compare_expression(bool allow_angle_operators = true)
        -> auto;

    //G relational-expression:
    //G     compare-expression
    //G     relational-expression '<'  compare-expression
    //G     relational-expression '>'  compare-expression
    //G     relational-expression '<=' compare-expression
    //G     relational-expression '>=' compare-expression
    //G
    auto relational_expression(bool allow_angle_operators = true)
        -> auto;

    //G equality-expression:
    //G     relational-expression
    //G     equality-expression '==' relational-expression
    //G     equality-expression '!=' relational-expression
    //G
    auto equality_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //G bit-and-expression:
    //G     equality-expression
    //G     bit-and-expression '&' equality-expression
    //G
    auto bit_and_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //G bit-xor-expression:
    //G     bit-and-expression
    //G     bit-xor-expression '^' bit-and-expression
    //G
    auto bit_xor_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //G bit-or-expression:
    //G     bit-xor-expression
    //G     bit-or-expression '|' bit-xor-expression
    //G
    auto bit_or_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //G logical-and-expression:
    //G     bit-or-expression
    //G     logical-and-expression '&&' bit-or-expression
    //G
    auto logical_and_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //  constant-expression:    // don't need intermediate production, just use:
    //  conditional-expression: // don't need intermediate production, just use:
    //G logical-or-expression:
    //G     logical-and-expression
    //G     logical-or-expression '||' logical-and-expression
    //G
    auto logical_or_expression(bool allow_angle_operators = true, bool allow_equality = true)
        -> auto;

    //G assignment-expression:
    //G     logical-or-expression
    //G     assignment-expression assignment-operator logical-or-expression
    //G
    auto assignment_expression(
        bool allow_angle_operators = true
    )
        -> std::unique_ptr<assignment_expression_node>;

    //G expression:               // eliminated 'condition:' - just use 'expression:'
    //G     assignment-expression
    //GTODO    try expression
    //G
    auto expression(
        bool allow_angle_operators = true,
        bool check_arrow           = true
    )
        -> std::unique_ptr<expression_node>;

    //G expression-list:
    //G     parameter-direction? expression
    //G     expression-list ',' parameter-direction? expression
    //G
    auto expression_list(
        token const* open_paren,
        bool inside_initializer = false
    )
        -> std::unique_ptr<expression_list_node>;


    //G type-id:
    //G     type-qualifier-seq? qualified-id
    //G     type-qualifier-seq? unqualified-id
    //G
    //G type-qualifier-seq:
    //G     type-qualifier
    //G     type-qualifier-seq type-qualifier
    //G
    //G type-qualifier:
    //G     'const'
    //G     '*'
    //G
    auto type_id()
        -> std::unique_ptr<type_id_node>;


    //G is-as-expression:
    //G     prefix-expression
    //G     is-as-expression is-type-constraint
    //G     is-as-expression is-value-constraint
    //G     is-as-expression as-type-cast
    //GTODO     type-id is-type-constraint
    //G
    //G is-type-constraint
    //G     'is' type-id
    //G
    //G is-value-constraint
    //G     'is' expression
    //G
    //G as-type-cast
    //G     'as' type-id
    //G
    auto is_as_expression()
        -> std::unique_ptr<is_as_expression_node>;


    //G unqualified-id:
    //G     identifier
    //G     keyword
    //G     template-id
    //GTODO     operator-function-id
    //G     ...
    //G
    //G template-id:
    //G     identifier '<' template-argument-list? '>'
    //G
    //G template-argument-list:
    //G     template-argument-list ',' template-argument
    //G
    //G template-argument:
    //G     # note: < > << >> are not allowed in expressions until new ( is opened
    //G     'const' type-id
    //G     expression
    //G     type-id
    //G
    auto unqualified_id()
        -> std::unique_ptr<unqualified_id_node>;


    //G qualified-id:
    //G     nested-name-specifier unqualified-id
    //G     member-name-specifier unqualified-id
    //G
    //G nested-name-specifier:
    //G     '::'
    //G     unqualified-id '::'
    //G
    //G member-name-specifier:
    //G     unqualified-id '.'
    //G
    auto qualified_id()
        -> std::unique_ptr<qualified_id_node>;


    //G id-expression:
    //G     qualified-id
    //G     unqualified-id
    //G
    auto id_expression()
        -> std::unique_ptr<id_expression_node>;

    //G literal:
    //G     integer-literal ud-suffix?
    //G     character-literal ud-suffix?
    //G     floating-point-literal ud-suffix?
    //G     string-literal ud-suffix?
    //G     boolean-literal ud-suffix?
    //G     pointer-literal ud-suffix?
    //G     user-defined-literal ud-suffix?
    //G
    auto literal()
        -> std::unique_ptr<literal_node>;

    //G expression-statement:
    //G     expression ';'
    //G     expression
    //G
    auto expression_statement(
        bool semicolon_required
    )
        -> std::unique_ptr<expression_statement_node>;


    //G selection-statement:
    //G     'if' 'constexpr'? logical-or-expression compound-statement
    //G     'if' 'constexpr'? logical-or-expression compound-statement 'else' compound-statement
    //G
    auto selection_statement()
        -> std::unique_ptr<selection_statement_node>;


    //G return-statement:
    //G     return expression? ';'
    //G
    auto return_statement()
        -> std::unique_ptr<return_statement_node>;


    //G iteration-statement:
    //G     label? 'while' logical-or-expression next-clause? compound-statement
    //G     label? 'do' compound-statement 'while' logical-or-expression next-clause? ';'
    //G     label? 'for' expression next-clause? 'do' unnamed-declaration
    //G
    //G label:
    //G     identifier ':'
    //G
    //G next-clause:
    //G     'next' assignment-expression
    //G
    auto iteration_statement()
        -> std::unique_ptr<iteration_statement_node>;


    //G alternative:
    //G     alt-name? is-type-constraint '=' statement
    //G     alt-name? is-value-constraint '=' statement
    //G     alt-name? as-type-cast '=' statement
    //G
    //GTODO alt-name:
    //G     unqualified-id ':'
    //G
    auto alternative()
        -> std::unique_ptr<alternative_node>;


    //G inspect-expression:
    //G     'inspect' 'constexpr'? expression '{' alternative-seq? '}'
    //G     'inspect' 'constexpr'? expression '->' type-id '{' alternative-seq? '}'
    //G
    //G alternative-seq:
    //G     alternative
    //G     alternative-seq alternative
    //G
    auto inspect_expression(bool is_expression)
        -> std::unique_ptr<inspect_expression_node>;


    //G jump-statement:
    //G     'break' identifier? ';'
    //G     'continue' identifier? ';'
    //G
    auto jump_statement()
        -> std::unique_ptr<jump_statement_node>;


    //G using-statement:
    //G     'using' id-expression ';'
    //G     'using' 'namespace' id-expression ';'
    //G
    auto using_statement()
        -> std::unique_ptr<using_statement_node>;


    //G statement:
    //G     selection-statement
    //G     using-statement
    //G     inspect-expression
    //G     return-statement
    //G     jump-statement
    //G     iteration-statement
    //G     compound-statement
    //G     declaration
    //G     expression-statement
    //G     contract
    //
    //GTODO     try-block
    //G
    auto statement(
        bool                     semicolon_required = true,
        source_position          equal_sign         = source_position{},
        bool                     parameters_allowed = false,
        compound_statement_node* compound_parent    = nullptr
    )
        -> std::unique_ptr<statement_node>;


    //G compound-statement:
    //G     '{' statement-seq? '}'
    //G
    //G statement-seq:
    //G     statement
    //G     statement-seq statement
    //G
    auto compound_statement(
        source_position equal_sign                      = source_position{},
        bool            allow_single_unbraced_statement = false
    )
        -> std::unique_ptr<compound_statement_node>;


    //G parameter-declaration:
    //G     this-specifier? parameter-direction? declaration
    //G
    //G parameter-direction: one of
    //G     'in' 'copy' 'inout' 'out' 'move' 'forward'
    //G
    //G this-specifier:
    //G     'implicit'
    //G     'virtual'
    //G     'override'
    //G     'final'
    //G
    auto parameter_declaration(
        bool is_returns   = false,
        bool is_named     = true,
        bool is_template  = true,
        bool is_statement = false
    )
        -> std::unique_ptr<parameter_declaration_node>;


    //G parameter-declaration-list
    //G     '(' parameter-declaration-seq? ')'
    //G
    //G parameter-declaration-seq:
    //G     parameter-declaration
    //G     parameter-declaration-seq ',' parameter-declaration
    //G
    auto parameter_declaration_list(
        bool is_returns    = false,
        bool is_named      = true,
        bool is_template   = false,
        bool is_statement  = false
    )
        -> std::unique_ptr<parameter_declaration_list_node>;


    //G contract:
    //G     '[' '[' contract-kind id-expression? ':' logical-or-expression ']' ']'
    //G     '[' '[' contract-kind id-expression? ':' logical-or-expression ',' string-literal ']' ']'
    //G
    //G contract-kind: one of
    //G     'pre' 'post' 'assert'
    //G
    auto contract()
        -> std::unique_ptr<contract_node>;


    //G function-type:
    //G     parameter-declaration-list throws-specifier? return-list? contract-seq?
    //G
    //G throws-specifier:
    //G     'throws'
    //G
    //G return-list:
    //G     expression-statement
    //G     '->' return-direction? type-id
    //G     '->' parameter-declaration-list
    //G
    //G return-direction: one of
    //G     'forward' 'move'
    //G
    //G contract-seq:
    //G     contract
    //G     contract-seq contract
    //G
    auto function_type(
        declaration_node* my_decl,
        bool              is_named = true
        )
        -> std::unique_ptr<function_type_node>;


    auto apply_type_metafunctions( declaration_node& decl )
        -> bool;


    //G unnamed-declaration:
    //G     ':' meta-functions-list? template-parameter-declaration-list? function-type requires-clause? '=' statement
    //G     ':' meta-functions-list? template-parameter-declaration-list? function-type statement
    //G     ':' meta-functions-list? template-parameter-declaration-list? type-id? requires-clause? '=' statement
    //G     ':' meta-functions-list? template-parameter-declaration-list? type-id
    //G     ':' meta-functions-list? template-parameter-declaration-list? 'final'? 'type' requires-clause? '=' statement
    //G     ':' 'namespace' '=' statement
    //G
    //G meta-functions-list:
    //G     '@' id-expression
    //G     meta-functions-list '@' id-expression
    //G
    //G requires-clause:
    //G      # note: for aliases, == is not allowed in expressions until new ( is opened
    //G      'requires' logical-or-expression
    //G
    //G template-parameter-declaration-list
    //G     '<' parameter-declaration-seq '>'
    //G
    auto unnamed_declaration(
        source_position                      start,
        bool                                 semicolon_required    = true,
        bool                                 captures_allowed      = false,
        bool                                 named                 = false,
        bool                                 is_parameter          = false,
        bool                                 is_template_parameter = false,
        std::unique_ptr<unqualified_id_node> id                    = {},
        accessibility                        access                = {},
        bool                                 is_variadic           = false,
        statement_node*                      my_stmt               = {}
    )
        -> std::unique_ptr<declaration_node>;


    //G alias:
    //G     ':' template-parameter-declaration-list? 'type' requires-clause? '==' type-id ';'
    //G     ':' 'namespace' '==' id-expression ';'
    //G     ':' template-parameter-declaration-list? type-id? requires-clause? '==' expression ';'
    //G
    //GT     ':' function-type '==' expression ';'
    //GT        # See commit 63efa6ed21c4d4f4f136a7a73e9f6b2c110c81d7 comment
    //GT        # for why I don't see a need to enable this yet
    //
    auto alias()
        -> std::unique_ptr<declaration_node>;


    //G declaration:
    //G     access-specifier? identifier '...'? unnamed-declaration
    //G     access-specifier? identifier alias
    //G
    //G access-specifier:
    //G     public
    //G     protected
    //G     private
    //G
    auto declaration(
        bool            semicolon_required    = true,
        bool            is_parameter          = false,
        bool            is_template_parameter = false,
        statement_node* my_stmt               = {}
    )
        -> std::unique_ptr<declaration_node>;


    //G declaration-seq:
    //G     declaration
    //G     declaration-seq declaration
    //G
    //G translation-unit:
    //G     declaration-seq?
    //
    auto translation_unit()
        -> std::unique_ptr<translation_unit_node>;

public:
    //-----------------------------------------------------------------------
    //  debug_print
    //
    auto debug_print(std::ostream& o)
        -> void;
};


//-----------------------------------------------------------------------
//
//  Common parts for printing visitors
//
//-----------------------------------------------------------------------
//
struct printing_visitor
{
    //-----------------------------------------------------------------------
    //  Constructor: remember a stream to write to
    //
    std::ostream& o;

    printing_visitor(std::ostream& out);
};


//-----------------------------------------------------------------------
//
//  Visitor for printing a parse tree
//
//-----------------------------------------------------------------------
//
class parse_tree_printer : printing_visitor
{
    using printing_visitor::printing_visitor;

public:
    auto start(token const& n, int indent) -> void;

    auto start(literal_node const&, int indent) -> void;

    auto start(expression_node const& n, int indent) -> void;

    auto start(expression_list_node::term const&n, int indent) -> void;

    auto start(expression_list_node const&, int indent) -> void;

    auto start(primary_expression_node const&, int indent) -> void;

    auto start(prefix_expression_node const&, int indent) -> void;

    auto start(is_as_expression_node const&, int indent) -> void;

    template<String Name, typename Term>
    auto start(binary_expression_node<Name, Term> const&, int indent) -> void;

    auto start(expression_statement_node const& n, int indent) -> void;

    auto start(postfix_expression_node const&, int indent) -> void;

    auto start(unqualified_id_node const&, int indent) -> void;

    auto start(qualified_id_node const&, int indent) -> void;

    auto start(type_id_node const&, int indent) -> void;

    auto start(id_expression_node const&, int indent) -> void;

    auto start(statement_node const&, int indent) -> void;

    auto start(compound_statement_node const&, int indent) -> void;

    auto start(selection_statement_node const& n, int indent) -> void;

    auto start(alternative_node const&, int indent) -> void;

    auto start(jump_statement_node const&, int indent) -> void;

    auto start(using_statement_node const& n, int indent) -> void;

    auto start(inspect_expression_node const& n, int indent) -> void;

    auto start(return_statement_node const&, int indent) -> void;

    auto start(iteration_statement_node const& n, int indent) -> void;

    auto start(contract_node const& n, int indent) -> void;

    auto start(type_node const&, int indent) -> void;

    auto start(namespace_node const&, int indent) -> void;

    auto start(function_type_node const& n, int indent) -> void;

    auto start(function_returns_tag const&, int indent) -> void;

    auto start(template_args_tag const&, int indent) -> void;

    auto start(declaration_identifier_tag const&, int indent) -> void;

    auto start(next_expression_tag const&, int indent) -> void;

    auto start(alias_node const& n, int indent) -> void;

    auto start(declaration_node const& n, int indent) -> void;

    auto start(parameter_declaration_node const& n, int indent) -> void;

    auto start(parameter_declaration_list_node const&, int indent) -> void;

    auto start(translation_unit_node const&, int indent) -> void;

    auto start(auto const&, int indent) -> void;

    auto end(auto const&, int) -> void;
};


}

#endif
