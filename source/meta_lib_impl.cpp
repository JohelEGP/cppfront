#include "cpp2util.h"
#include <map>

namespace cpp2::meta {

static std::vector<record> registry;

register_function::register_function(const char* name, mf_signature_in* f) {
	std::cout << "\nRegistering function (in)    -> " << name << "...";
	registry.emplace_back(record{name, f});
}

register_function::register_function(const char* name, mf_signature_inout* f) {
	std::cout << "\nRegistering function (inout) -> " << name << "...";
	registry.emplace_back(record{name, f});
}

register_function::~register_function() {
	std::cout << "\nCalling register_function destructor";
}

}

void* cpp2_meta_registry_(size_t* registry_size) {
	*registry_size = ::cpp2::meta::registry.size();
	return static_cast<void*>(::cpp2::meta::registry.data());
}
CPP2_C_API constexpr auto cpp2_meta_registry = &cpp2_meta_registry_;
