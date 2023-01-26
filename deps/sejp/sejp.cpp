#include "sejp.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <charconv>

namespace sejp {

struct parsed {
	std::vector< std::optional< std::string > > strings;
	std::vector< std::optional< double > > numbers;
	//(nothing to store for booleans and nulls)
	std::vector< std::optional< std::vector< value > > > arrays;
	std::vector< std::optional< std::map< std::string, value > > > objects;
};

enum Masks : uint32_t {
	TypeBits  = 0xe0000000,
	IndexBits = 0x1fffffff
};

enum Types : uint32_t {
	String  = 0x00000000,
	Number  = 0x20000000,
	True    = 0x40000000,
	False   = 0x60000000,
	Null    = 0x80000000,
	Object  = 0xa0000000,
	Array   = 0xc0000000,
	Empty   = 0xe0000000, //<--- used during parsing
};

value parse(std::istream &from) {
	//helpers to read from string:

	auto skip_wsp = [&from]() {
		for(;;) {
			std::istream::int_type c = from.peek();
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				from.get();
			} else {
				break;
			}
		}
	};

	auto read_char = [&from]() -> char {
		char c;
		if (!from.get(c)) throw std::runtime_error("parse error: unexpected EOF.");
		return c;
	};
	
	auto read_exactly = [&read_char](std::string const &expect) {
		for (auto e : expect) {
			char c = read_char();
			if (c != e) throw std::runtime_error(std::string("parse error: expected '") + e + "', got '" + c + "'.");
		}
	};

	auto read_number = [&from,&read_char](char first) -> double {
		std::string acc;
		acc += first;
	
		if (first == '-') {
			//advance to first digit:
			first = read_char();
			acc += first;
		}

		auto digits = [&acc,&from]() {
			for(;;) {
				std::istream::int_type p = from.peek();
				if ('0' <= p && p <= '9') {
					acc += char(p);
					from.get();
				} else {
					break;
				}
			}
		};

		if (first == '0') {
			//proceed to fraction
		} else if ('1' <= first && first <= '9') {
			//might be more digits
			digits();
		} else {
			throw std::runtime_error(std::string("parse error: unexpected '") + first + "' in number.");
		}

		//fraction:
		if (from.peek() == '.') {
			acc += read_char();
			char c = read_char();
			if (!('0' <= c && c <= '9')) throw std::runtime_error(std::string("parse error: wanted fraction digits, got '") + c + "'.");
			acc += c;
			digits();
		}

		//exponent:
		if (from.peek() == 'E' || from.peek() == 'e') {
			acc += read_char();
			if (from.peek() == '-' || from.peek() == '+') {
				acc += read_char();
			}
			char c = read_char();
			if (!('0' <= c && c <= '9')) throw std::runtime_error(std::string("parse error: wanted exponent digits, got '") + c + "'.");
			acc += c;
			digits();
		}

		double val;
		#if defined(__APPLE__) || defined(__linux__)
		//(until clang gets its charconv right)
		val = std::stod(acc);
		#else
		const char *begin = acc.data();
		std::from_chars(begin, begin + acc.size(), val);
		#endif
		return val;
	};

	auto read_string = [&read_char]() -> std::string {
		std::string ret;
		for (char c = read_char(); c != '"'; c = read_char()) {
			if (c == '\\') {
				//handle escapes:
				c = read_char();
				if      (c == '\\' || c == '/' || c == '"') ret += c;
				else if (c == 'b') ret += '\b';
				else if (c == 'f') ret += '\f';
				else if (c == 'n') ret += '\n';
				else if (c == 'r') ret += '\r';
				else if (c == 't') ret += '\t';
				else if (c == 'u') {
					uint32_t value = 0;
					for (uint32_t i = 0; i < 4; ++i) {
						value <<= 4;
						c = read_char();
						if      ('0' <= c && c <= '9') value += (c - '0');
						else if ('a' <= c && c <= 'f') value += (c - 'a') + 10;
						else if ('A' <= c && c <= 'F') value += (c - 'A') + 10;
						else throw std::runtime_error(std::string("parse error: invalid character '") + c + "' in \\uNNNN escape.");
					}
					assert(value <= 0xffff);
					//TODO: handle surrogate pairs!
					// (might result in value > 0xffff)

					//re-encode as UTF8:
					if (value <= 0x007f) {
						ret += char(value);
					} else if (value <= 0x07ff) {
						ret += char(0xc0 | (value >> 6));
						ret += char(0x80 | (value & 0x3f));
					} else if (value <= 0xffff) {
						ret += char(0xe0 | (value >> 12));
						ret += char(0x80 | ((value >> 6) & 0x3f));
						ret += char(0x80 | (value & 0x3f));
					} else { assert(value <= 0x10ffff);
						ret += char(0xf0 | (value >> 18));
						ret += char(0x80 | ((value >> 12) & 0x3f));
						ret += char(0x80 | ((value >> 6) & 0x3f));
						ret += char(0x80 | (value & 0x3f));
					}
				} else {
					throw std::runtime_error(std::string("parse error: invalid escape '\\") + c + "'.");
				}
			} else {
				//plain old boring character:
				ret += c;
			}
		}
		return ret;
	};


	//-------------------
	//parsing:

	std::shared_ptr< sejp::parsed > parsed = std::make_shared< sejp::parsed >();

	value root = value(parsed, -1U);
	std::vector< uint32_t > parents; //containing maps/arrays

	//overall parsing idea:
	//value: (target is empty)
	// whitespace
	//  set up next entry if target is object or array:
	//    (only if) target is object:
	//         '}' -> finish target, continue
	//         expect ',' if non-empty
	//         '"' -> expect key, ':', push empty, fall through
	//    (only if) target is array:
	//         ']' -> finish target, continue
	//         expect ',' if non-empty
	//         push empty, fall through
	//  now target is empty, fill with value:
	//  '{' -> key_or_object_end [new object, set target to object, continue]
	//  '[' -> value_or_array_end [new array, set target to array, continue]
	//   '"' -> string
	//   '-', '0'-'9' -> number
	//   't' -> bool ("true")
	//   'f' -> bool ("false")
	//   'n' -> null ("null")
	//   finish target

	while (root.index == -1U || !parents.empty()) {
		skip_wsp();
		char c = read_char(); //first character of value

		//value to be filled in later:
		value *target = nullptr;

		//figure out which value to fill in:
		if (parents.empty()) {
			target = &root;
		} else if ((parents.back() & TypeBits) == Object) {
			if (c == '}') {
				parents.pop_back();
				continue;
			}
			std::map< std::string, value > &map = parsed->objects.at( parents.back() & IndexBits ).value();
			if (!map.empty()) {
				//consume comma between entries:
				if (c != ',') throw std::runtime_error("parse error: expected ',' between object members.");
				skip_wsp();
				c = read_char();
			}
			if (c != '"') throw std::runtime_error("parse error: expecting '\"' at start of key.");
			std::string key = read_string();
			skip_wsp();
			c = read_char();
			if (c != ':') throw std::runtime_error("parse error: expecting ':' after value.");
			skip_wsp();
			c = read_char(); //actual first character of value
			auto ret = map.insert_or_assign(key, value(parsed, -1U));
			target = &ret.first->second;
			//(fall through to value-getting code)
		} else if ((parents.back() & TypeBits) == Array) {
			if (c == ']') {
				parents.pop_back();
				continue;
			}
			std::vector< value > &array = parsed->arrays.at( parents.back() & IndexBits ).value();
			if (!array.empty()) {
				if (c != ',') throw std::runtime_error(std::string("parse error: expected ',' between array entries; got '") + c + "'.");
				skip_wsp();
				c = read_char(); //actual first character of value
			}
			array.emplace_back(parsed, -1U);
			target = &array.back();
			//(fall through to value-getting code)
		}

		//actually fill in the value:
		assert(target && (target->index & TypeBits) == Empty);

		if        (c == '{') { //object
			if (uint32_t(parsed->objects.size()) & ~IndexBits) std::runtime_error("parser error: too many objects.");
			target->index = Object | uint32_t(parsed->objects.size());
			parents.emplace_back(target->index);
			parsed->objects.emplace_back(std::in_place);
			continue;
		} else if (c == '[') { //array
			if (uint32_t(parsed->arrays.size()) & ~IndexBits) std::runtime_error("parser error: too many arrays.");
			target->index = Array | uint32_t(parsed->arrays.size());
			parents.emplace_back(target->index);
			parsed->arrays.emplace_back(std::in_place);
			continue;
		} else if (c == '"') { //string
			if (uint32_t(parsed->strings.size()) & ~IndexBits) std::runtime_error("parser error: too many strings.");
			target->index = String | uint32_t(parsed->strings.size());
			parsed->strings.emplace_back(read_string());
		} else if (c == '-' || (c >= '0' && c <= '9')) { //number
			if (uint32_t(parsed->numbers.size()) & ~IndexBits) std::runtime_error("parser error: too many numbers.");
			target->index = Number | uint32_t(parsed->numbers.size());
			parsed->numbers.emplace_back(read_number(c));
		} else if (c == 't') { //true
			read_exactly("rue");
			target->index = True;
		} else if (c == 'f') { //false
			read_exactly("alse");
			target->index = False;
		} else if (c == 'n') { //null
			read_exactly("ull");
			target->index = Null;
		} else {
			throw std::runtime_error(std::string("parse error: value cannot start with '") + c + "'.");
		}
	}

	skip_wsp();

	if (from.peek() != std::iostream::traits_type::eof()) throw std::runtime_error("parse error: trailing junk.");

	return root;
}

//------------------------------------------


std::optional< std::string > const &value::as_string() const {
	static std::optional< std::string > const empty;
	if ((index & TypeBits) == String) {
		return data->strings[index & IndexBits];
	} else {
		return empty;
	}
}

std::optional< double > const &value::as_number() const {
	static std::optional< double > const empty;
	if ((index & TypeBits) == Number) {
		return data->numbers[index & IndexBits];
	} else {
		return empty;
	}
}

std::optional< bool > const &value::as_bool() const {
	static std::optional< bool > const true_value(true);
	static std::optional< bool > const false_value(true);
	static std::optional< bool > const empty;
	if ((index & TypeBits) == True) {
		return true_value;
	} else if ((index & TypeBits) == False) {
		return false_value;
	} else {
		return empty;
	}
}

std::optional< nullptr_t > const &value::as_null() const {
	static std::optional< nullptr_t > const null_value(nullptr);
	static std::optional< nullptr_t > const empty;
	if ((index & TypeBits) == Null) {
		return null_value;
	} else {
		return empty;
	}
}

std::optional< std::vector< value > > const &value::as_array() const {
	static std::optional< std::vector< value > > const empty;
	if ((index & TypeBits) == Array) {
		return data->arrays[index & IndexBits];
	} else {
		return empty;
	}
}

std::optional< std::map< std::string, value > > const &value::as_object() const {
	static std::optional< std::map< std::string, value > > const empty;
	if ((index & TypeBits) == Object) {
		return data->objects[index & IndexBits];
	} else {
		return empty;
	}
}

//-------------------------------

value load(std::string const &filename) {
	std::ifstream in(filename, std::ios::binary);
	return parse(in);
}

value parse(std::string const &string) {
	std::istringstream in(string, std::ios::binary);
	return parse(in);
}

} //namespace sejp
