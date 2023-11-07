module;

#define CPP2_MODULE

#include "cpp2reflect.h"

export module cpp2.reflect;

namespace cpp2 {

export using cpp2::passing_style;

namespace meta {

  export using meta::declaration;
  export using meta::function_declaration;
  export using meta::object_declaration;
  export using meta::type_declaration;
  export using meta::alias_declaration;

  export using meta::add_virtual_destructor;
  export using meta::interface;
  export using meta::polymorphic_base;
  export using meta::ordered;
  export using meta::weakly_ordered;
  export using meta::partially_ordered;
  export using meta::copyable;
  export using meta::basic_value;
  export using meta::value;
  export using meta::weakly_ordered_value;
  export using meta::partially_ordered_value;
  export using meta::cpp2_struct;
  export using meta::cpp2_enum;
  export using meta::flag_enum;
  export using meta::cpp2_union;
  export using meta::print;

} // namespace meta
} // namespace cpp2
