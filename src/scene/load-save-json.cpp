#include "scene.h"
#include "animator.h"
#include "../rasterizer/sample_pattern.h"
#include "../util/to_json.h"

#include <sejp/sejp.hpp>

#include <typeinfo>
#include <filesystem>

//---------------------------------------------------------
//Actual classes used with introspection for load/save:

//base class -- used to book-keep where we are in introspection:
struct HasIntrospectionStack {

	struct IntrospectionFrame;
	std::vector< IntrospectionFrame const * > introspection_stack;
	struct IntrospectionFrame {
		IntrospectionFrame(HasIntrospectionStack &self_, std::string const &name_, const char *type_)
		: self(self_), name(name_), type(type_) {
			//std::cout << "-> " << type << " " << name << std::endl; //DEBUG
			self.introspection_stack.emplace_back(this);
		}

		template< typename T >
		IntrospectionFrame(HasIntrospectionStack &self_, std::string const &name_, T const &t)
		: self(self_), name(name_), type(name_type(&t,0)) {
			//std::cout << "-> " << type << " " << name << std::endl; //DEBUG
			self.introspection_stack.emplace_back(this);
		}
		~IntrospectionFrame() {
			assert(!self.introspection_stack.empty());
			assert(self.introspection_stack.back() == this);
			self.introspection_stack.pop_back();
		}
		HasIntrospectionStack &self;
		std::string const &name;
		std::string type;

		//overloads to do type naming:
		template< typename T, typename C = std::remove_cv_t< std::remove_reference_t< T > >, typename enable = decltype(C::TYPE) >
		static std::string name_type(T const *, int) {
			return C::TYPE;
		}

		//write down names for some types that don't have introspection:
		#define NAME(cls, name) \
			static std::string name_type(cls const *, int) { \
				return name; \
			}

		NAME(Halfedge_Mesh, "Halfedge_Mesh");
		NAME(HDR_Image, "HDR_Image");
		NAME(Spectrum, "Spectrum");
		NAME(Vec4, "Vec4");
		NAME(Vec3, "Vec3");
		NAME(Vec2, "Vec2");
		NAME(Quat, "Quat");
		NAME(bool, "bool");
		NAME(float, "float");
		NAME(uint32_t, "uint32_t");
		NAME(std::string, "string");
		NAME(SamplePattern const *, "SamplePattern");

		#undef NAME

		//some standard classes:
		template< typename T >
		static std::string name_type(std::vector< T > const *, int) {
			T const *t = nullptr;
			return "vector<" + name_type(t,0) + ">";
		}

		template< typename T >
		static std::string name_type(std::weak_ptr< T > const *, int) {
			T const *t = nullptr;
			return "weak_ptr<" + name_type(t,0) + ">";
		}

		template< typename T >
		static std::string name_type(std::unordered_map< std::string, std::shared_ptr< T > > const *, int) {
			T const *t = nullptr;
			return "unordered_map<string,shared_ptr<" + name_type(t,0) + ">>";
		}

		/*
		//generic fall-through -- useful for development, but not the prettiest names:
		template< typename T, typename C = std::remove_cv_t< std::remove_reference_t< T > > >
		static std::string name_type(T const *, double) {
			return typeid(T).name();
		}
		*/

	};

	std::string introspection_str() const {
		std::string str;
		for (auto f : introspection_stack) {
			if (!str.empty()) str += ".";
			str += "[" + std::string(f->type) + " " + f->name + "]";
		}
		return str;
	}
};


struct JSONLoader : HasIntrospectionStack {
	static void load(sejp::value const &value, std::string const &from_path, Scene &scene) {
		JSONLoader loader(scene, from_path);

		ValueFrame frame(loader, "", value);

		introspect< Intent::Write >(loader, scene);
	}

	JSONLoader(Scene &scene_, std::string const &from_path_) : scene(scene_), from_path(from_path_) { }
	Scene &scene;
	std::string from_path;

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
	void for_member(std::string const &name, std::function< void(sejp::value const &) > const &op) {
		auto object = value_stack.back()->value.as_object();
		if (!object) {
			std::cerr << "cannot load " << introspection_str() << " from " << value_str() << " -- it is not an object." << std::endl;
			return;
		}
		auto f = object->find(name);
		if (f == object->end()) {
			std::cerr << "cannot load " << introspection_str() << " from " << value_str() << " -- it does not have a '" << name << "' property." << std::endl;
			return;
		}
		ValueFrame frame(*this, "." + name, f->second);
		op(f->second);
	}

	//if current value is an object, iterate all members:
	void for_members(std::function< void(std::string const &, sejp::value const &) > const &op) {
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

	//if current value is an array, iterate all elements:
	void for_elements(std::function< void(uint32_t i, sejp::value const &) > const &op) {
		auto array = value_stack.back()->value.as_array();
		if (!array) {
			std::cerr << "cannot load " << introspection_str() << " by iterating " << value_str() << " -- it is not an array." << std::endl;
			return;
		}
		uint32_t i = 0;
		for (auto const &v : *array) {
			ValueFrame frame(*this, "[" + std::to_string(i) + "]", v);
			op(i, v);
			++i;
		}
	}

	//----------------------------------------------------------------------------------
	//generic introspection helpers:

	//sometimes need to branch depending on whether a 'from_json' function exists:
	template< typename T, typename enable = decltype(from_json(*(sejp::value*)nullptr, (T*)nullptr) ) >
	void from_json_introspect_or_complain(sejp::value const &value, T &t, char) {
		try { 
			from_json(value, &t);
		} catch (std::runtime_error const &e) {
			std::cerr << "Failed to load " << value_str() << " -> " << introspection_str() << ": " << e.what() << "; using default-constructed value instead." << std::endl;
			t = T();
		}
	}

	template< typename T >
	void from_json_introspect_or_complain(sejp::value const &, T &t, int) {
		introspect< Intent::Write >(*this, t);
	}

	template< typename T >
	void from_json_introspect_or_complain(std::string const &name, T const &t, double) {
		std::cout << "Cannot introspect " << introspection_str() << " -> " << name << " (at " << value_str() << ")." << std::endl;
	}

	//load a Storage< > of something:
	template< typename T >
	void operator()(std::string const &name, std::unordered_map< std::string, std::shared_ptr< T > > &out) {
		IntrospectionFrame frame(*this, name, out);
		if (!out.empty()) {
			std::cerr << "WARNING: loading into non-empty " << introspection_str() << "." << std::endl;
		}
		for_member(name, [&]( sejp::value const & ) {
			//pre-allocate:
			for_members( [&]( std::string const &key, sejp::value const &value ) {
				out.emplace(key, std::make_shared< T > ());
			});
			//fill:
			for_members( [&]( std::string const &key, sejp::value const &value ) {
				from_json_introspect_or_complain(value, *out.at(key), 'x');
			});
		});
	}

	//load a vector< > of something:
	template< typename T >
	void operator()(std::string const &name, std::vector< T > &out) {
		IntrospectionFrame frame(*this, name, out);
		if (!out.empty()){
			std::cerr << "WARNING: loading into non-empty " << introspection_str() << " -- will clear." << std::endl;
			out.clear();
		}
		for_member(name, [&]( sejp::value const & ) {
			//pre-allocate:
			for_elements( [&]( uint32_t i, sejp::value const &value ) {
				out.emplace_back();
			});
			//fill:
			for_elements( [&]( uint32_t i, sejp::value const &value ) {
				from_json_introspect_or_complain(value, out.at(i), 'x');
			});
		});
	}

	//weak reference to something:
	template< typename T >
	void operator()(std::string const &name, std::weak_ptr< T > &ref) {
		IntrospectionFrame frame(*this, name, ref);
		ref.reset();
		for_member(name, [&,this]( sejp::value const &val ){
			//leave empty on null:
			if (val.as_null()) return;
			auto str = val.as_string();
			if (!str) {
				std::cerr << "Cannot load " << introspection_str() << " from " << value_str() << " -- not null or a string. (Will leave empty.)" << std::endl;
				return;
			}
			Scene::Storage< T > const &storage = scene.get_storage< T >();
			auto f = storage.find(*str);
			if (f == storage.end()) {
				std::cerr << "Cannot load " << introspection_str() << " from " << value_str() << " -- '" << *str << "' is not in storage." << std::endl;
				return;
			}
			ref = f->second;
		});
	}

	//- - - - - - - - - - - - - - - - -
	//special data types:
	void operator()(std::string const &name, HDR_Image &t) {
		IntrospectionFrame frame(*this, name, t);
		for_member(name, [&,this]( sejp::value const &val ) {
			auto str = val.as_string();
			if (!str) {
				std::cerr << "Cannot load " << introspection_str() << " from " << value_str() << " -- not a string. (Will set to missing image.)" << std::endl;
				t = HDR_Image::missing_image();
				return;
			}
			if (str->substr(0,6) == "hdr64:") {
				std::vector< uint8_t > buffer;
				try {
					from_json_base64(val, &buffer, "hdr64:");
					t = HDR_Image::decode(buffer.data(), buffer.size());
				} catch (std::exception const &e) {
					std::cout << "Failed to load " << introspection_str() << " as a base64-encoded data blob: " << e.what() << std::endl;
				}
				return;
			}
			std::string fn = ( std::filesystem::absolute(std::filesystem::path(from_path)).remove_filename() / std::filesystem::path(*str) ).generic_string();
			try {
				t = HDR_Image::load(fn);
				return;
			} catch ( std::exception const &e ) {
				std::cout << "Failed to load " << introspection_str() << " from " << fn << ": " << e.what() << std::endl;
				std::cout << "Trying as non-relative path..." << std::endl;
			}
			try {
				t = HDR_Image::load(*str);
				return;
			} catch ( std::exception const &e ) {
				std::cout << "Failed to load " << introspection_str() << " from " << *str << ": " << e.what() << std::endl;
			}
			warn("Image '%s' is missing.", str->c_str());
			t = HDR_Image::missing_image();
			t.loaded_from = fn; //somewhat awkward way to set this!
		});
	}

	//- - - - - - - - - - - - - - - - -
	//everything else:

	template< typename T >
	void operator()(std::string const &name, T&& t) {
		IntrospectionFrame frame(*this, name, t);
		for_member(name, [&]( sejp::value const &val ){
			from_json_introspect_or_complain(val, t, 'x');
		});
	}
};


struct JSONSaver : HasIntrospectionStack {
	static void save(Scene const &scene, std::ostream &to, std::string const &to_path) {
		JSONSaver saver(to, to_path);

		IntrospectionFrame(saver, "scene", scene);
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
		IntrospectionFrame frame(*this, name, val);
		std::vector< std::string > keys; keys.reserve(val.size());
		//store refs, populate keys:
		for (auto const &[k, v] : val) {
			keys.emplace_back(k);
			auto ret = json_refs.emplace(v.get(), to_json(k));
			if (!ret.second) {
				std::cerr << "WARNING: " << introspection_str() << " re-uses name '" << k << "' -- references may be screwed up." << std::endl;
			}
		}

		//store items as object members: (in sorted order)
		std::sort(keys.begin(), keys.end());
		member(name, [&,this](){
			object([&,this](){
				for (auto const &k : keys) {
					(*this)(k, *val.at(k));
				}
			});
		});
	}

	//save a weak reference by turning into a string (using our stored list of references):
	template< typename T >
	void operator()(std::string const &name, std::weak_ptr< T > const &ref) {
		IntrospectionFrame frame(*this, name, ref);
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

	//handle vectors of stuff as arrays:
	template< typename T >
	void operator()(std::string const &name, std::vector< T > const &val) {
		IntrospectionFrame frame(*this, name, val);

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

	//handle HDR_Image by saving relative path
	void operator()(std::string const &name, HDR_Image const &val) {
		IntrospectionFrame frame(*this, name, "HDR_Image");
		member_value(name, [&,this](){
			if (val.loaded_from == "") {
				std::cerr << "WARNING: HDR_Image does not indicate where it was loaded from. Saving a (pretty large!) base64 encoded blob into the file." << std::endl;
				std::vector< uint8_t > buffer = val.encode();
				to << to_json_base64(buffer, "hdr64:");
			} else {
				std::string rel = std::filesystem::proximate( std::filesystem::path(val.loaded_from), std::filesystem::absolute(std::filesystem::path(to_path)).remove_filename() ).generic_string();
				std::cout << val.loaded_from << " relative to " << to_path << " is " << rel << std::endl; //DEBUG
				to << to_json(rel);
			}
		});
	}

	//- - - - - - - - - - - - - - - - -
	//anything not otherwise mentioned, either look for a from_json/to_json, store as an object by introspecting, or complain:

	//quick two-overloads hack to allow complaining about unimplemented introspection fn:

	template< Intent I, typename T, typename enable = decltype(to_json(T())) >
	void convert_introspect_or_complain(std::string const &name, T const &t, char) {
		IntrospectionFrame frame(*this, name, t);
		member_value(name, [&,this](){ to << to_json(t); });
	}


	template< Intent I, typename T >
	void convert_introspect_or_complain(std::string const &name, T const &t, int) {
		IntrospectionFrame frame(*this, name, t);

		member(name, [&,this](){
			object([&,this](){
				introspect< I >(*this, t);
			});
		});

	}

	template< Intent I, typename T >
	void convert_introspect_or_complain(std::string const &name, T const &t, double) {
		std::cout << "Cannot introspect " << introspection_str() << " -> " << name << "." << std::endl;
	}

	template< typename T >
	void operator()(std::string const &name, T const & t) {
		convert_introspect_or_complain< Intent::Read >(name, t, 'x');
	}

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

	auto const &top = from.as_object();
	if (!top) throw std::runtime_error("Expecting object at top level of animator json.");

	if (!(top->count("FORMAT") && top->at("FORMAT").as_string() && *top->at("FORMAT").as_string() == "js3d-v1")) {
		warn("Missing or unexpected 'FORMAT' when reading animator. (Ignoring and continuing.)");
	}

	if (!(top->count("splines") && top->at("splines").as_object())) {
		warn("Missing or non-object 'splines' when reading animator. (Result will be empty.)");
		return animator;
	}

	auto &splines = top->at("splines").as_object();
	assert(splines); //already checked above!
	for (auto const &rv : *splines) {
		std::string const &resource = rv.first;
		auto const &channels = rv.second.as_object();
		if (!channels) {
			warn("Ignoring non-object resource %s", resource.c_str());
			continue;
		}
		for (auto const &cv : *channels) {
			std::string const &channel = cv.first;
			try {
				auto const &data = cv.second.as_object();
				if (!data) throw std::runtime_error("not an object");

				if (!(data->count("type") && data->at("type").as_string())) throw std::runtime_error("type not string");
				std::string type = *data->at("type").as_string();

				//create correct type of spline to hold the data:
				Animator::Channel_Spline spline;
				if (type == "bool") spline = Spline< bool >();
				else if (type == "float") spline = Spline< float >();
				else if (type == "Vec2") spline = Spline< Vec2 >();
				else if (type == "Vec3") spline = Spline< Vec3 >();
				else if (type == "Vec4") spline = Spline< Vec4 >();
				else if (type == "Quat") spline = Spline< Quat >();
				else if (type == "Spectrum") spline = Spline< Spectrum >();
				else if (type == "Mat4") spline = Spline< Mat4 >();
				else throw std::runtime_error("unrecognized type '" + type + "'");

				if (!(data->count("knots") && data->at("knots").as_array())) throw std::runtime_error("knots not array");
				auto const &knots = *data->at("knots").as_array();

				//copy knots into the spline:
				std::visit([&](auto&& s){
					bool warned = false;
					for (uint32_t i = 0; i + 1 < knots.size(); i += 2) {
						try {
							auto const &time = knots[i].as_number();
							if (!time) throw std::runtime_error("time is not number");

							decltype(s.at(0.0f)) val;
							from_json(knots[i+1], &val);

							s.knots.emplace(float(*time), val);
						} catch (std::runtime_error const &e) {
							if (!warned) {
								warn("Ignoring knot(s) in %s.%s: %s", resource.c_str(), channel.c_str(), e.what());
								warned = true;
							}
						}
					}
				}, spline);

				//store into animator:
				animator.splines.emplace(std::make_pair(resource, channel), std::move(spline));
			} catch (std::runtime_error const &e) {
				warn("Ignoring %s.%s: %s", resource.c_str(), channel.c_str(), e.what());
			}
		}
	}

	return animator;
}

void Animator::save_json(std::ostream &to) const {
	//arrange paths in sorted order:
	std::map< std::string, std::set< std::string > > paths;
	for (auto const &kv : splines) {
		paths[kv.first.first].emplace(kv.first.second);
	}
	to << '{';
	to << "\"FORMAT\":\"js3d-v1\"";
	to << ",\n\"splines\":{";
	bool first_resource = true;
	for (auto const &resource_channels : paths) {
		if (first_resource) first_resource = false;
		else to << ',';
		to << "\n\t" << to_json(resource_channels.first) << ":{";
		bool first_channel = true;
		for (auto const &channel : resource_channels.second) {
			if (first_channel) first_channel = false;
			else to << ',';
			to << "\n\t\t" << to_json(channel) << ":{";

			Path path = std::make_pair(resource_channels.first, channel);
			Channel_Spline const &spline = splines.at(path);

			to << "\n\t\t\t" << "\"type\":\"";
			std::visit(overloaded{
				[&](Spline< bool > const &){ to << "bool"; },
				[&](Spline< float > const &){ to << "float"; },
				[&](Spline< Vec2 > const &){ to << "Vec2"; },
				[&](Spline< Vec3 > const &){ to << "Vec3"; },
				[&](Spline< Vec4 > const &){ to << "Vec4"; },
				[&](Spline< Quat > const &){ to << "Quat"; },
				[&](Spline< Spectrum > const &){ to << "Spectrum"; },
				[&](Spline< Mat4 > const &){ to << "Mat4"; }
			}, spline);
			to << '"';

			to << ",\n\t\t\t" << "\"knots\":[";
			std::visit([&](auto&& s){
				//write array of alternating times and values:
				bool first = true;
				for (auto const &knot : s.knots) {
					if (first) first = false;
					else to << ",";
					to << "\n\t\t\t\t" << to_json(knot.first) << ", " << to_json(knot.second);
				}
			}, spline);
			to << "\n\t\t\t]";

			to << "\n\t\t}";

		}
		to << "\n\t" << '}';
	}
	to << '}';

	to << '}';
}
