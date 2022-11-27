#pragma once

#include "../lib/log.h"

#include <variant>
#include <vector>
#include <algorithm>
#include <cassert>

//Scene's introspection system is built on each relevant item in a Scene providing either:
// (1) an `introspect` member function:
//   template< Intent intent, typename F >
//   void introspect(F &&f);
//   which invokes `f("member", &member)` for every member
// (2) (Maybe, eventually) some fancy overloaded structure or something

enum class Intent {
	Read,   //reading data (will not mutate)
	Write,  //writing data (may mutate, throw if invalid)
	Animate //reading/writing channels for animation
};

//if you want to fail at runtime instead of compile time on introspecting:
#ifdef INTROSPECT_SOFT_FAILURE
//as per https://slashvar.github.io/2019/08/17/Detecting-Functions-existence-with-SFINAE.html
namespace details {
	template< Intent I, typename F, typename T >
	auto introspect_impl(F&& f, T&& t, int) -> decltype(std::remove_reference< T >::type:: template introspect< I >(f, t)) {
		std::remove_reference< T >::type:: template introspect< I >(std::forward< F >(f), std::forward< T >(t));
	}

	template< Intent I, typename F, typename T >
	void introspect_impl(F&& f, T&& t, float) {
		warn("Trying to introspect %s, but no method available.", typeid(T).name());
	}
};
#endif

template< Intent I, typename F, typename T >
void introspect(F&& f, T&& t) {
#ifdef INTROSPECT_SOFT_FAILURE
	//will *complain* when run if introspection not implemented for T:
	details::introspect_impl< I >(std::forward< F >(f), std::forward< T >(t), 0);
#else
	//will fail at compile time if introspection not implemented for T:
	std::remove_reference< T >::type:: template introspect< I >(std::forward< F >(f), std::forward< T >(t));
#endif
}

//-------------------------------------------------------------
//Introspect helper for variants:
// on read - introspects the type then the current alternative
// on animate - introspects the current alternative
// on write - introspects the type, updates the alternative, then introspects the new alternative


template< Intent I, typename F, typename V, /*size_t sz = std::variant_size_v< std::remove_reference_t< V > >,*/ std::enable_if_t< I == Intent::Read || I == Intent::Animate, bool > = true >
void introspect_variant(F&& f, V&& v) {
	std::visit([&](auto&& t){
		std::string type;
		type = std::remove_reference_t< decltype(t) >::TYPE;
		if constexpr (I != Intent::Animate) f("type", type);
		introspect< I >(std::forward< F >(f), t);
	}, v);
}

namespace details {
	template< typename T >
	using clean_t = std::remove_cv_t< std::remove_reference_t< T > >;

	//base case:
	template< Intent I, size_t idx, typename V, typename F, std::enable_if_t< idx >= std::variant_size_v< clean_t< V > >, bool> = true >
	void write_type(F&& f, V&& v, std::string const &type) {
		//ran out of variants, so complain + use first option:
		using T = std::variant_alternative_t< 0, clean_t< V > >;
		warn("Type '%s' does not appear in variant -- will substitute '%s'.", type.c_str(), T::TYPE);
		if (!std::holds_alternative< T >(v)) v = T();
		introspect< I >(std::forward< F >(f), std::get< T >(v));
	}

	//inductive case:
	template< Intent I, size_t idx, typename V, typename F, std::enable_if_t< idx < std::variant_size_v< clean_t< V > >, bool> = true >
	void write_type(F&& f, V&& v, std::string const &type) {
		using T = std::variant_alternative_t< idx, clean_t< V > >;
		if (type == T::TYPE) {
			if (!std::holds_alternative< T >(v)) v = T();
			introspect< I >(std::forward< F >(f), std::get< T >(v));
		} else {
			write_type< I, idx + 1 >(std::forward< F >(f), std::forward< V >(v), type);
		}
	}

};


template< Intent I, typename F, typename V, std::enable_if_t< I == Intent::Write, bool > = true >
void introspect_variant(F&& f, V&& v) {
	std::string type;
	std::visit([&](auto&& t){
		type = std::remove_reference_t< decltype(t) >::TYPE;
	}, v);
	f("type", type);
	details::write_type< I, 0 >(std::forward< F >(f), std::forward< V >(v), type);
}

//-------------------------------------------------------------
//introspect helper for enums:
// translates enum to string using table, introspects the string, converts back to enum using table

template< Intent I, typename F, typename E, typename E2 >
void introspect_enum(F&& f, const char *name, E&& e, std::vector< std::pair< const char *, E2 > > const& possible) {
	assert(!possible.empty());

	std::string value;
	{ //find current name from supplied list:
		auto found = std::find_if(possible.begin(), possible.end(), [&](auto&& p){ return p.second == e; });
		assert(found != possible.end());
		value = found->first;
	}
	f(name, value);
	if constexpr (I == Intent::Write) {
		//translate string back to enum:
		auto found = std::find_if(possible.begin(), possible.end(), [&](auto&& p){ return p.first == value; });
		if (found == possible.end()) {
			warn("Invalid enum value '%s' (for %s); setting to '%s'.", value.c_str(), name, possible[0].first);
			found = possible.begin();
		}
		e = found->second;
	}
}
