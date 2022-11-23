#pragma once

//A "Somewhat Eager JSON Parser" that parses and converts files
//upon loading into some lists of numbers, objects, bools, nulls;
//then provides a generic "value" handle to the root.

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace sejp {
	//sejp::parsed represents the results of scanning a JSON file:
	struct parsed;
	
	//generic value:
	struct value {
		//internals:
		std::shared_ptr< parsed const > data;
		uint32_t index; //(opaque) index data's value storage
		value(std::shared_ptr< parsed const > const &data_, uint32_t index_) : data(data_), index(index_) { }

		//interface:
		//  NOTE: these functions take O(1) time
		std::optional< std::string > const &as_string() const;
		std::optional< double > const &as_number() const;
		std::optional< bool > const &as_bool() const;
		std::optional< nullptr_t > const &as_null() const;
		std::optional< std::vector< value > > const &as_array() const;
		std::optional< std::map< std::string, value > > const &as_object() const;
	};

	//how you make values:
	//  NOTE: O(length of data) time, space.
	//  NOTE: loaded data is retained via shared_ptr until values referring to it go out of scope
	//  NOTE: throws on parse error
	value load(std::string const &filename);
	value parse(std::string const &string);

} //namespace sljp
