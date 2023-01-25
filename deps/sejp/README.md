This code was written by Jim McCann (ix), and is released under the MIT license. The latest version of this code is available at [https://github.com/ixchow/sejp](https://github.com/ixchow/sejp).

# Somewhat Eager JSON Parser

SEJP is a Somewhat Eager JSON Parser for C++ -- it reads JSON file from a stream and makes it into dynamically-typed values as it does so.
It is a small and reasonably lightweight piece of code (easy to audit and integrate!).

However, it does parse and hold the entire file in memory; so is not particularly suited to streaming access of very large files.

## Usage

Usage example:
```cpp
#include "sejp.hpp"
#include <iostream>

int main(int argc, char **argv) {
	std::string json =
		"{\n"
		"	\"version\":15,\n"
		"	\"places\":[\n"
		"		\"Pittsburgh, PA\","
		"		\"Salt Lake City, UT\""
		"	],\n"
		"	\"extra\":{\n"
		"		\"optimized\":false,\n"
		"		\"debug\":true\n"
		"	},\n"
		"	\"more\":null\n"
		"}\n"
	;
	sejp::value val = sejp::parse(json);

	{ //simple example -- if you know what you are accessing:
		//if you are sure: (will throw if key is missing)
		double version = val.as_object().value().at("version").as_number().value();
		std::cout << "Version is " << version << "." << std::endl;
	}

	//if you try to get the value of something at the wrong type, an exception is thrown:
	try {
		//NOTE: this will fail!
		double inversion = val.as_object().value().at("inversion").as_number().value();
		std::cout << "ERROR -- got " << inversion << " instead of failing." << std::endl;
	} catch (std::exception &e) {
		std::cout << "SUCCESS -- key \"inversion\" doesn't exist. (Reported as: " << e.what() << ".)" << std::endl;
	}

	// a more complete example:

	//you can do this because the returned optionals are valid as long as some value from the parse is held:
	std::map< std::string, sejp::value > const &object = val.as_object().value();
	double version = object.at("version").as_number().value();
	std::cout << "Version is " << version << std::endl;

	//if a "places" key exists...
	if (auto places = object.find("places"); places != object.end()) {
		//...coerce to an array (will throw if not) and iterate:
		for (auto const &place : places->second.as_array().value()) {
			std::cout << "  Place: " << place.as_string().value() << std::endl;
		}
	}

}
```

Values keep a `shared_ptr` to the parsed data, and return references into these arrays. This means that one can safely hold references to (e.g.) `value.as_array().value()` as long as `value` remains alive.


## Installation

Copy `sejp.hpp` and `sejp.cpp` into your project.

Make sure to build with (at least) c++17 support, since this code uses `std::optional< >`.
