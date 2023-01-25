#include "sejp.hpp"

#include <iostream>

//print value on one or more lines, each of which starts with prefix and the last of which ends with suffix (and a newline):
void dump(sejp::value const &value, std::string prefix = "", std::string suffix = "", std::string const *key = nullptr) {
	auto dump_string = [](std::string const &str) {
		std::cout << '"';
		for (auto c : str) {
			if (c == '\\' || c == '"') std::cout << '\\';
			std::cout << c;
		}
		std::cout << '"';
	};
	std::cout << prefix;
	if (key) {
		dump_string(*key);
		std::cout << ':';
	}
	if (auto val = value.as_string()) {
		dump_string(*val);
	} else if (auto val = value.as_number()) {
		std::cout << *val;
	} else if (auto val = value.as_bool()) {
		std::cout << (*val ? "true" : "false");
	} else if (auto val = value.as_null()) {
		std::cout << "null";
	} else if (auto val = value.as_array()) {
		std::cout << '[';
		for (auto const &child : *val) {
			if (&child == &val->front()) std::cout << '\n';
			dump(child, prefix + "    ", (&child != &val->back() ? "," : ""));
		}
		if (!val->empty()) std::cout << prefix;
		std::cout << ']';
	} else if (auto val = value.as_object()) {
		std::cout << '{';
		for (auto const &child : *val) {
			if (&child == &*val->begin()) std::cout << '\n';
			dump(child.second, prefix + "     ", (&child != &*val->rbegin() ? "," : ""), &child.first);
		}
		if (!val->empty()) std::cout << prefix;
		std::cout << '}';
	} else {
		std::cout << "Value outside of all categories?!" << std::endl;
		exit(1);
	}
	std::cout << suffix << std::endl;
}

int main(int argc, char **argv) {

	//a bunch of simple cases:
	dump(sejp::parse("\"\""));
	dump(sejp::parse("\"hello world\""));
	dump(sejp::parse("0"));
	dump(sejp::parse("3.1"));
	//dump(sejp::parse(".1")); //<-- not a valid number
	dump(sejp::parse("-3.1e4"));
	dump(sejp::parse("1e3"));
	dump(sejp::parse("-1E+3"));
	dump(sejp::parse("1.2E-3"));
	dump(sejp::parse("true"));
	dump(sejp::parse("false"));
	dump(sejp::parse("null"));
	dump(sejp::parse("[]"));
	dump(sejp::parse("[1,2,3]"));
	dump(sejp::parse("[1,\"b\",3]"));
	dump(sejp::parse("[[2,4],\"b\",[5]]"));
	dump(sejp::parse("[[2,4],\"b\",[5,[\"six\"],[\"seven\"]]]"));
	dump(sejp::parse("{}"));
	dump(sejp::parse("{\"key\":\"value\"}"));
	dump(sejp::parse("{\"b\":1,\"a\":\"two\",\"b\":\"this is the real value of b\"}"));

	dump(sejp::parse("[{\"a\":null,\"this\":[4]},\"b\",{}, {\"5\":5},[\"six\"],\"seven\"]"));

	//TODO: some complex cases!

	return 0;
}
