#include "scene.h"
#include "animator.h"
#include "../rasterizer/sample_pattern.h"

#include <sejp/sejp.hpp>

#include <charconv>
#include <typeinfo>
#include <filesystem>

//some conversion helpers for writing json values:

[[maybe_unused]]
static std::string to_json(std::string const &str) {
	std::string ret;
	ret += '"';
	for (auto const &c : str) {
		if (c == '\\' || c == '"') ret += '\\';
		ret += c;
	}
	ret += '"';
	return ret;
}

[[maybe_unused]]
static std::string to_json(float val) {
	double dval = val; //NOTE: convert to double here because on the json side number will be parsed as a double, and we want correct round-trip behavior without having to think too hard
	std::array< char, 30 > buffer; //shouldn't need more than 24 digits
	//after example at: https://en.cppreference.com/w/cpp/utility/to_chars
	auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), dval);
	assert(ec == std::errc());
	return std::string(buffer.data(), ptr);
}

[[maybe_unused]]
static std::string to_json(Spectrum const &spec) {
	return '[' + to_json(spec.r) + ',' + to_json(spec.g) + ',' + to_json(spec.b) + ']';
}

[[maybe_unused]]
static std::string to_json(Vec3 const &vec) {
	return '[' + to_json(vec.x) + ',' + to_json(vec.y) + ',' + to_json(vec.z) + ']';
}

[[maybe_unused]]
static void from_json(std::string const &desc, std::map< std::string, sejp::value > const &info, std::string const &key, Vec3 *vec_) {
	assert(vec_);
	auto &vec = *vec_;
	auto f = info.find(key);
	if (f == info.end()) throw std::runtime_error(desc + "." + key + " does not exist.");
	auto const &arr = f->second.as_array();
	if (!arr) throw std::runtime_error(desc + "." + key + " is not an array.");
	if (arr->size() != 3) throw std::runtime_error(desc + "." + key + " has " + std::to_string(arr->size()) + " values, expected 3.");
	auto const &x = arr->at(0).as_number();
	auto const &y = arr->at(1).as_number();
	auto const &z = arr->at(2).as_number();
	if (!x || !y || !z) throw std::runtime_error(desc + "." + key + " has non-number entries.");
	vec.x = *x;
	vec.y = *y;
	vec.z = *z;
}

[[maybe_unused]]
static std::string to_json(Quat const &quat) {
	return '[' + to_json(quat.x) + ',' + to_json(quat.y) + ',' + to_json(quat.z) + ',' + to_json(quat.w) + ']';
}

[[maybe_unused]]
static void from_json(std::string const &desc, std::map< std::string, sejp::value > const &info, std::string const &key, Quat *quat_) {
	assert(quat_);
	auto &quat = *quat_;
	auto f = info.find(key);
	if (f == info.end()) throw std::runtime_error(desc + "." + key + " does not exist.");
	auto const &arr = f->second.as_array();
	if (!arr) throw std::runtime_error(desc + "." + key + " is not an array.");
	if (arr->size() != 4) throw std::runtime_error(desc + "." + key + " has " + std::to_string(arr->size()) + " values, expected 4.");
	auto const &x = arr->at(0).as_number();
	auto const &y = arr->at(1).as_number();
	auto const &z = arr->at(2).as_number();
	auto const &w = arr->at(3).as_number();
	if (!x || !y || !z || !w) throw std::runtime_error(desc + "." + key + " has non-number entries.");
	quat.x = *x;
	quat.y = *y;
	quat.z = *z;
	quat.w = *w;
}


//base class -- used to book-keep where we are in introspection:
struct HasIntrospectionStack {
	struct IntrospectionFrame;
	std::vector< IntrospectionFrame const * > introspection_stack;
	struct IntrospectionFrame {
		IntrospectionFrame(HasIntrospectionStack &self_, std::string const &name_, std::type_info const &type_) : self(self_), name(name_), type(type_) {
			self.introspection_stack.emplace_back(this);
		}
		~IntrospectionFrame() {
			assert(!self.introspection_stack.empty());
			assert(self.introspection_stack.back() == this);
			self.introspection_stack.pop_back();
		}
		HasIntrospectionStack &self;
		std::string const &name;
		std::type_info const &type;
	};

	std::string introspection_str() const {
		std::string str;
		for (auto f : introspection_stack) {
			if (!str.empty()) str += ".";
			str += "[" + std::string(f->type.name()) + " " + f->name + "]";
		}
		return str;
	}
};


struct JSONLoader : HasIntrospectionStack {
	static void load(sejp::value const &value, std::string const &from_path, Scene &scene) {
		JSONLoader loader(scene);

		ValueFrame frame(loader, "", value);

		introspect< Intent::Write >(loader, scene);
	}

	JSONLoader(Scene &scene_) : scene(scene_) { }
	Scene &scene;

	//-------------------------------------------------
	//This object traverses an introspection hierarchy and a json hierarchy.
	//Track both for ease of error reporting:

	struct ValueFrame;
	std::vector< ValueFrame * > value_stack;
	struct ValueFrame {
		ValueFrame(JSONLoader &loader_, std::string &&name_, sejp::value const &value_) : loader(loader_), name(std::move(name_)), value(value_) {
			loader.value_stack.emplace_back(this);
		}
		~ValueFrame() {
			assert(!loader.value_stack.empty());
			assert(loader.value_stack.back() == this);
			loader.value_stack.pop_back();
		}
		JSONLoader &loader;
		std::string name;
		sejp::value const &value;
	};

	std::string value_str() const {
		std::string str;
		for (auto f : value_stack) {
			str += f->name;
		}
		return str;
	}

	//----------------------------------------------------------------------------------
	//Some helpers to make it easier to walk through the value hierarchy:

	//if current value is an object with a property 'name', traverse to it and run 'op':
	void for_property(std::string const &name, std::function< void(sejp::value const &) > const &op) {
		auto object = value_stack.back()->value.as_object();
		if (!object) {
			std::cerr << "cannot load " << introspection_str() << " from " << value_str() << " -- it is not an object." << std::endl;
			return;
		}
		auto f = object->find(name);
		if (f == object->end()) {
			std::cerr << "cannot load " << introspection_str() << " from " << value_str() << " -- it does name have a '" << name << "' property." << std::endl;
			return;
		}
		ValueFrame frame(*this, "." + name, f->second);
		op(f->second);
	}

	//if current value is an object, iterate all properties:
	void for_properties(std::function< void(std::string const &, sejp::value const &) > const &op) {
		auto object = value_stack.back()->value.as_object();
		if (!object) {
			std::cerr << "cannot load " << introspection_str() << " by iterating " << value_str() << " -- it is not an object." << std::endl;
			return;
		}
		for (auto const &[k,v] : *object) {
			ValueFrame frame(*this, "." + k, v);
			op(k, v);
		}
	}

	//----------------------------------------------------------------------------------
	//generic introspection helpers:

	//load a Storage< > of something:
	template< typename T >
	void operator()(std::string const &name, std::unordered_map< std::string, std::shared_ptr< T > > &out) {
		IntrospectionFrame frame(*this, name, typeid(T));
		if (!out.empty()) {
			std::cerr << "WARNING: loading into non-empty " << introspection_str() << "." << std::endl;
		}
		for_property(name, [&]( sejp::value const & ) {
			//pre-allocate:
			for_properties( [&]( std::string const &key, sejp::value const &value ) {
				out.emplace(key, std::make_shared< T > ());
			});
			//fill:
			for_properties( [&]( std::string const &key, sejp::value const &value ) {
				introspect< Intent::Write >(*this, *out.at(key));
			});
		});
	}

	//handle instances:
	void operator()(std::string const &name, decltype(Scene::instances) &instances) {
		IntrospectionFrame frame(*this, name, typeid(decltype(Scene::instances)));
		for_property(name, [&]( sejp::value const & ){
			introspect< Intent::Write >(*this, instances);
		});
	}

	//references to transforms:
	void operator()(std::string const &name, std::weak_ptr< Transform > &ref) {
		IntrospectionFrame frame(*this, name, typeid(decltype(ref)));
		ref.reset();
		for_property(name, [&,this]( sejp::value const &val ){
			//leave empty on null:
			if (val.as_null()) return;
			auto str = val.as_string();
			if (!str) {
				std::cerr << "Cannot load " << introspection_str() << " from " << value_str() << " -- not null or a string. (Will leave empty.)" << std::endl;
				return;
			}
			auto f = scene.transforms.find(*str);
			if (f == scene.transforms.end()) {
				std::cerr << "Cannot load " << introspection_str() << " from " << value_str() << " -- '" << *str << "' is not a transform." << std::endl;
				return;
			}
			ref = f->second;
		});
	}

	//- - - - - - - - - - - - - - - - -
	//basic data:

	//Vec3 from [x,y,z] array
	void operator()(std::string const &name, Vec3 &vec) {
		IntrospectionFrame frame(*this, name, typeid(Vec3));
		for_property(name, [&]( sejp::value const &val ){
			auto arr = val.as_array();
			if (!arr) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- not an array.");
			}
			if (arr->size() != 3) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- has " + std::to_string(arr->size()) + " values, expected 3.");
			}
			auto const &x = arr->at(0).as_number();
			auto const &y = arr->at(1).as_number();
			auto const &z = arr->at(2).as_number();
			if (!x || !y || !z) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- non-number entries.");
			}
			vec.x = *x;
			vec.y = *y;
			vec.z = *z;
		});
	}

	//Quat from [x,y,z,w] array
	void operator()(std::string const &name, Quat &quat) {
		IntrospectionFrame frame(*this, name, typeid(Quat));
		for_property(name, [&]( sejp::value const &val ){
			auto arr = val.as_array();
			if (!arr) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- not an array.");
			}
			if (arr->size() != 4) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- has " + std::to_string(arr->size()) + " values, expected 4.");
			}
			auto const &x = arr->at(0).as_number();
			auto const &y = arr->at(1).as_number();
			auto const &z = arr->at(2).as_number();
			auto const &w = arr->at(3).as_number();
			if (!x || !y || !z || !w) {
				throw std::runtime_error("Cannot load " + introspection_str() + " from " + value_str() + " -- non-number entries.");
			}
			quat.x = *x;
			quat.y = *y;
			quat.z = *z;
			quat.w = *w;
		});
	}

	//- - - - - - - - - - - - - - - - -
	//fall-through case:
	template< typename T >
	void operator()(std::string const &name, T&& t) {
		IntrospectionFrame frame(*this, name, typeid(T));
		std::cout << "Loader unimplemented for " << introspection_str() << "'." << std::endl;
	}
};


struct JSONSaver : HasIntrospectionStack {
	static void save(Scene const &scene, std::ostream &to, std::string const &to_path) {
		JSONSaver saver(to, to_path);

		IntrospectionFrame(saver, "scene", typeid(Scene));
		saver.object([&](){
			saver.member_value("FORMAT", [&](){ saver.to << "\"js3d-v1\""; });
			introspect< Intent::Read >(saver, scene);
		});
	}

	JSONSaver(std::ostream &to_, std::string const &to_path_) : to(to_), to_path(to_path_) {
		json_refs.emplace(nullptr, "null");
	}
	std::ostream &to;
	std::string to_path;

	Scene const *scene = nullptr;

	//brute force way of getting a string / null to refer to anything:
	std::unordered_map< void const *, std::string > json_refs;

	//---------------------------------------
	// helpers for printing json nicely

	//Context tracks where in the structure the output code is:
	enum class Context {
		Value, None, //wants a single value, or got a single value and wants nothing
		FirstMember, NthMember, //in object (first, non-first)
		FirstElement, NthElement, //in array (first, non-first)
	};

	Context context = Context::Value; //root of the file expects a value
	std::string indent = "";
	bool pretty = true;

	//callback is going to write a value:
	void value(std::function< void() > &&callback) {
		Context after = Context::None;
		if (context == Context::Value) after = Context::None;
		else if (context == Context::FirstElement || context == Context::NthElement) after = Context::NthElement;
		else std::cerr << "Unexpected context for value() at " << introspection_str() << "." << std::endl;
		
		if (context == Context::FirstElement) {
			if (pretty) to << '\n' << indent;
		} else if (context == Context::NthElement) {
			to << ',';
			if (pretty) to << '\n' << indent;
		}

		context = Context::None; //don't want any further structure inside value
		callback();

		context = after;
	}

	//callback is going to call member() N times to fill in object:
	void object(std::function< void() > &&callback) {
		value([&,this](){
			to << '{';
			if (pretty) indent += '\t';
			context = Context::FirstMember;
			callback();
			if (pretty) indent.erase(indent.size()-1);
			if (pretty && context != Context::FirstMember) to << '\n' << indent;
			to << '}';
		});
	}

	//callback is going to call value() N times to fill in array:
	void array(std::function< void() > &&callback) {
		value([&,this](){
			to << '[';
			if (pretty) indent += '\t';
			context = Context::FirstElement;
			callback();
			if (pretty) indent.erase(indent.size()-1);
			if (pretty && context != Context::FirstElement) to << '\n' << indent;
			to << ']';
		});
	}

	//callback is going to call value() to fill in member's value:
	void member(std::string const &name, std::function< void() > &&callback) {
		if (context == Context::FirstMember) {
			if (pretty) to << '\n' << indent;
		} else if (context == Context::NthMember) {
			to << ',';
			if (pretty) to << '\n' << indent;
		}
		to << to_json(name) << ':';
		context = Context::Value;
		callback();
		//expecting: Context::None
		context = Context::NthMember;
	}

	//because it comes up a lot:
	void member_value(std::string const &name, std::function< void() > &&callback) {
		member(name, [&](){ value( std::move(callback) ); });
	}


	//---------------------------------------
	// actual saving stuff

	//save a Storage< > of something:
	template< typename T >
	void operator()(std::string const &name, std::unordered_map< std::string, std::shared_ptr< T > > const &val) {
		IntrospectionFrame frame(*this, name, typeid(T));
		//store refs:
		for (auto const &[k, v] : val) {
			auto ret = json_refs.emplace(v.get(), to_json(k));
			if (!ret.second) {
				std::cerr << "WARNING: " << introspection_str() << " re-uses name '" << k << "' -- references may be screwed up." << std::endl;
			}
		}
		//store items:
		member(name, [&,this](){
			object([&,this](){
				for (auto const &[k, v] : val) {
					(*this)(k, *v.get());
				}
			});
		});
	}

	//save a weak reference to something:
	template< typename T >
	void operator()(std::string const &name, std::weak_ptr< T > const &ref) {
		IntrospectionFrame frame(*this, name, typeid(T));
		member(name, [&,this](){
			value([&,this](){
				auto f = json_refs.find(ref.lock().get());
				if (f == json_refs.end()) {
					std::cerr << "WARNING: " << introspection_str() << " references something outside the scene -- will save as null" << std::endl;
					to << "null";
				} else {
					to << f->second;
				}
			});
		});
	}

	//handle vectors of stuff:
	template< typename T >
	void operator()(std::string const &name, std::vector< T > const &val) {
		IntrospectionFrame frame(*this, name, typeid(std::vector< T >));

		member(name, [&,this](){
			array([&,this](){
				for (auto const &i : val) {
					object([&,this](){
						introspect< Intent::Read >(*this, i);
					});
				}
			});
		});
	}

	//- - - - - - - - - - - - - - - - -
	//fancy data types (or that are big enough to require special care):

	//sample patterns get saved by name:
	void operator()(std::string const &name, SamplePattern const *val) {
		IntrospectionFrame frame(*this, name, typeid(SamplePattern));
		if (val == nullptr) {
			warn("%s has a null sample pattern. Not saving it.", introspection_str().c_str());
			return;
		}

		member(name, [&,this](){
			value([&,this](){
				to << to_json(val->name);
			});
		});
	}

	//Halfedge Meshes get custom save code based on s3ds format:
	void operator()(std::string const &name, Halfedge_Mesh const &val) {
		IntrospectionFrame frame(*this, name, typeid(Halfedge_Mesh));

		member(name, [&,this](){
			object([&,this](){
				member("type", [&,this](){ to << "\"js3d-e/2-1\""; });

				//TODO
				std::cerr << "About that halfedge mesh saving code." << std::endl;
				
			});
		});
	}

	//handle HDR_Image by saving relative path
	void operator()(std::string const &name, HDR_Image const &val) {
		IntrospectionFrame frame(*this, name, typeid(HDR_Image));
		member_value(name, [&,this](){
			if (val.loaded_from == "") {
				std::cerr << "HDR_Image does not indicate where it was loaded from. Cannot save as relative path." << std::endl;
				//TODO: store as in-file base64 blob?
			} else {
				std::string rel = std::filesystem::proximate( std::filesystem::path(val.loaded_from), std::filesystem::path(to_path).remove_filename() ).generic_string();
				std::cout << val.loaded_from << " relative to " << to_path << " is " << rel << std::endl; //DEBUG
				to << to_json(rel);
			}
		});
	}

	//- - - - - - - - - - - - - - - - -
	//basic data types:

	void operator()(std::string const &name, bool const &val) {
		member_value(name, [&,this](){
			to << (val ? "true" : "false");
		});
	}

	void operator()(std::string const &name, uint32_t const &val) {
		member_value(name, [&,this](){
			to << val;
		});
	}

	void operator()(std::string const &name, float const &val) {
		member_value(name, [&,this](){
			to << to_json(val);
		});
	}

	void operator()(std::string const &name, Vec3 const &val) {
		member_value(name, [&,this](){
			to << to_json(val);
		});
	}

	void operator()(std::string const &name, Quat const &val) {
		member_value(name, [&,this](){
			to << to_json(val);
		});
	}

	void operator()(std::string const &name, Spectrum const &val) {
		member_value(name, [&,this](){
			to << to_json(val);
		});
	}

	void operator()(std::string const &name, std::string const &val) {
		member_value(name, [&,this](){
			to << to_json(val);
		});
	}

	//- - - - - - - - - - - - - - - - -
	//anything not otherwise mentioned, we store as an object by introspecting:
	template< typename T >
	void operator()(std::string const &name, T const & t) {
		IntrospectionFrame frame(*this, name, typeid(T));

		member(name, [&,this](){
			object([&,this](){
				introspect< Intent::Read >(*this, t);
			});
		});
	}

	/*
	//- - - - - - - - - - - - - - - - -
	//fall-through case:
	template< typename T >
	void operator()(std::string const &name, T const & t) {
		IntrospectionFrame frame(*this, name, typeid(T));
		std::cout << "Saver unimplemented for " << introspection_str() << "'." << std::endl;
	}
	*/

};



Scene Scene::load_json(sejp::value const &from, std::string const &from_path) {
	Scene scene;
	JSONLoader::load(from, from_path, scene);
	return scene;
}

void Scene::save_json(std::ostream &to, std::string const &to_path) const {
	JSONSaver::save(*this, to, to_path);
}

//---------------------------------------------------------

Animator Animator::load_json(sejp::value const &from) {
	Animator animator;
	//TODO
	warn("Loading for animator is not yet implemented.");
	return animator;
}

void Animator::save_json(std::ostream &to) const {
	//TODO
	warn("Saving for animator is not yet implemented -- writing an empty object.");
	to << "{}";
}
