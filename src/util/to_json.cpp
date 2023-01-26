#include "to_json.h"

#include "../geometry/halfedge.h"
#include "../rasterizer/sample_pattern.h"
#include "../lib/mathlib.h"

#include <sejp/sejp.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <charconv>
#include <unordered_set>

std::string to_json(std::string const &str) {
	std::string ret;
	ret += '"';
	for (auto const &c : str) {
		if (c == '\\' || c == '"') {
			ret += '\\';
			ret += c;
		} else if (c == '\b') {
			ret += '\\'; ret += 'b';
		} else if (c == '\f') {
			ret += '\\'; ret += 'f';
		} else if (c == '\n') {
			ret += '\\'; ret += 'n';
		} else if (c == '\r') {
			ret += '\\'; ret += 'r';
		} else if (c == '\t') {
			ret += '\\'; ret += 't';
		} else if (c >= 0 && c <= 20) {
			ret += '\\';
			ret += 'u';
			ret += '0';
			ret += '0';
			ret += "0123456789abcdef"[(c >> 8) & 0xf];
			ret += "0123456789abcdef"[c & 0xf];
		} else {
			ret += c;
		}
	}
	ret += '"';
	return ret;
}
void from_json(sejp::value const &info, std::string *val) {
	auto string = info.as_string();
	if (!string) throw std::string("not a string");
	*val = *string;
}


std::string to_json(bool val) {
	return (val ? "true" : "false");
}
void from_json(sejp::value const &info, bool *val) {
	auto boolean = info.as_bool();
	if (!boolean) throw std::string("not a boolean");
	*val = *boolean;
}


std::string to_json(uint32_t val) {
	return std::to_string(val);
}
void from_json(sejp::value const &info, uint32_t *val) {
	auto number = info.as_number();
	if (!number) throw std::string("not a number");
	double n = std::round(std::clamp(*number, 0.0, double(std::numeric_limits< uint32_t >::max())));
	if (n != *number) {
		std::cerr << "WARNING: converted " << *number << " to " << n << " when loading uint32_t" << std::endl;
	}
	*val = uint32_t(n);
}


std::string to_json(float val) {
	double dval = val; //NOTE: convert to double here because on the json side number will be parsed as a double, and we want correct round-trip behavior without having to think too hard
	#if defined(__APPLE__) || defined(__linux__)
	//until clang gets its charconv right
	std::ostringstream oss;
	oss.precision( std::numeric_limits< double >::digits10 + 1 );
	oss << dval;
	return oss.str();
	#else
	std::array< char, 30 > buffer; //shouldn't need more than 24 digits
	//after example at: https://en.cppreference.com/w/cpp/utility/to_chars
	auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), dval);
	assert(ec == std::errc());
	return std::string(buffer.data(), ptr);
	#endif
}
void from_json(sejp::value const &info, float *val) {
	auto number = info.as_number();
	if (!number) throw std::string("not a number");
	*val = float(*number);
}


template< uint32_t N >
std::string to_json(float const (&arr)[N]) {
	std::ostringstream ret;
	ret << '[';
	bool first = true;
	for (auto v : arr) {
		if (first) first = false;
		else ret << ',';
		ret << to_json(v);
	}
	ret << ']';
	return ret.str();
}
template< uint32_t N >
void from_json(sejp::value const &info, float (*arr)[N]) {
	auto array = info.as_array();
	if (!array) throw std::runtime_error("not an array");
	if (array->size() != N) throw std::runtime_error("expected " + std::to_string(N) + " values, got " + std::to_string(array->size()));
	float got[N];
	try {
		for (uint32_t i = 0; i < N; ++i) {
			from_json(array->at(i), &got[i]);
		}
	} catch (std::exception const &e) {
		throw std::runtime_error(std::string("error getting number: ") + e.what());
	}
	for (uint32_t i = 0; i < N; ++i) {
		(*arr)[i] = got[i];
	}
}


std::string to_json(Vec2 const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Vec2 *val) {
	from_json(info, &val->data);
}


std::string to_json(Vec3 const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Vec3 *val) {
	from_json(info, &val->data);
}


std::string to_json(Vec4 const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Vec4 *val) {
	from_json(info, &val->data);
}


std::string to_json(Mat4 const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Mat4 *val) {
	from_json(info, &val->data);
}


std::string to_json(Spectrum const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Spectrum *val) {
	from_json(info, &val->data);
}


std::string to_json(Quat const &val) {
	return to_json(val.data);
}
void from_json(sejp::value const &info, Quat *val) {
	from_json(info, &val->data);
}

std::string to_json(SamplePattern const *val) {
	if (val == nullptr) {
		std::cerr << "WARNING: trying to store a null sample pattern, will store as empty string." << std::endl;
		return to_json("");
	} else {
		//store sample patterns by name:
		return to_json(val->name);
	}
}
void from_json(sejp::value const &info, SamplePattern const **val) {
	std::string name;
	from_json(info, &name);

	std::vector< SamplePattern > const &all = SamplePattern::all_patterns();
	auto f = std::find_if(all.begin(), all.end(), [&](SamplePattern const &s) { return s.name == name; });
	if (f == all.end()) throw std::runtime_error("pattern \"" + name + "\" is unknown");
	*val = &*f;
}

std::string to_json(Halfedge_Mesh const &mesh) {

	std::unordered_map< Halfedge_Mesh::HalfedgeCRef, uint32_t > halfedge_to_index;
	halfedge_to_index.reserve(mesh.halfedges.size());

	//start by sorting mesh halfedges into twinned pairs: (with the first of the pair being the one the Edge points to)
	for (auto h = mesh.halfedges.begin(); h != mesh.halfedges.end(); ++h) {
		bool h_added, t_added;
		if (h->edge->halfedge == h) {
			h_added = halfedge_to_index.emplace(h, uint32_t(halfedge_to_index.size())).second;
			t_added = halfedge_to_index.emplace(h->twin, uint32_t(halfedge_to_index.size())).second;
		} else {
			t_added = halfedge_to_index.emplace(h->twin, uint32_t(halfedge_to_index.size())).second;
			h_added = halfedge_to_index.emplace(h, uint32_t(halfedge_to_index.size())).second;
		}
		if (h_added != t_added) {
			std::cout << "Strange: edge and twin were somehow not added at the same time." << std::endl;
		}
	}

	std::ostringstream str;
	str << "{ \"FORMAT\":\"s3d-hm-1\"";

	{ //halfedge data:
		std::vector< uint32_t > halfedge_nexts(mesh.halfedges.size());
		std::vector< Vec2 > halfedge_corner_uvs(mesh.halfedges.size());
		std::vector< Vec3 > halfedge_corner_normals(mesh.halfedges.size());

		for (auto h = mesh.halfedges.begin(); h != mesh.halfedges.end(); ++h) {
			uint32_t idx = halfedge_to_index.at(h);
			halfedge_nexts[idx] = halfedge_to_index.at(h->next);
			halfedge_corner_uvs[idx] = h->corner_uv;
			halfedge_corner_normals[idx] = h->corner_normal;
		}

		str << ",\"halfedge_nexts\":" << to_json_base64(halfedge_nexts, "uint32:");
		str << ",\"halfedge_corner_uvs\":" << to_json_base64(halfedge_corner_uvs, "vec2:");
		str << ",\"halfedge_corner_normals\":" << to_json_base64(halfedge_corner_normals, "vec3:");
	}

	{ //vertex data:
		std::vector< uint32_t > bone_weight_bones;
		std::vector< float > bone_weight_weights;

		std::vector< uint32_t > vertex_halfedges;
		std::vector< Vec3 > vertex_positions;
		std::vector< uint32_t > vertex_bone_weight_ends; //(begin is previous vertex's end, so not storing)

		for (auto v = mesh.vertices.begin(); v != mesh.vertices.end(); ++v) {
			vertex_halfedges.emplace_back(halfedge_to_index.at(v->halfedge));
			vertex_positions.emplace_back(v->position);
			for (auto bw : v->bone_weights) {
				bone_weight_bones.emplace_back(bw.bone);
				bone_weight_weights.emplace_back(bw.weight);
			}
			vertex_bone_weight_ends.emplace_back(uint32_t(bone_weight_bones.size()));
		}

		assert(bone_weight_bones.size() == bone_weight_weights.size());

		assert(vertex_halfedges.size() == mesh.vertices.size());
		assert(vertex_positions.size() == mesh.vertices.size());
		assert(vertex_bone_weight_ends.size() == mesh.vertices.size());

		str << ",\"bone_weight_bones\":" << to_json_base64(bone_weight_bones, "uint32:");
		str << ",\"bone_weight_weights\":" << to_json_base64(bone_weight_weights, "float:");

		str << ",\"vertex_halfedges\":" << to_json_base64(vertex_halfedges, "uint32:");
		str << ",\"vertex_positions\":" << to_json_base64(vertex_positions, "vec3:");
		str << ",\"vertex_bone_weight_ends\":" << to_json_base64(vertex_bone_weight_ends, "uint32:");
	}

	{ //edge data:
		std::vector< bool > edge_sharps;
		edge_sharps.resize(mesh.halfedges.size() / 2, false);

		for (auto e = mesh.edges.begin(); e != mesh.edges.end(); ++e) {
			uint32_t idx = halfedge_to_index.at(e->halfedge);
			if (idx % 2 != 0) warn("Edge pointing to odd halfedge; this should not happen.");
			edge_sharps.at(idx/2) = e->sharp;
		}

		str << ",\"edge_sharps\":" << to_json_base64(edge_sharps, "bool:");
	}
	
	{ //face data:
		std::vector< uint32_t > face_halfedges;
		std::vector< bool > face_boundaries;

		for (auto f = mesh.faces.begin(); f != mesh.faces.end(); ++f) {
			face_halfedges.emplace_back(halfedge_to_index.at(f->halfedge));
			face_boundaries.emplace_back(f->boundary);
		}
		str << ",\"face_halfedges\":" << to_json_base64(face_halfedges, "uint32:");
		str << ",\"face_boundaries\":" << to_json_base64(face_boundaries, "bool:");
	}

	str << "}";

	return str.str();
}
void from_json(sejp::value const &info, Halfedge_Mesh *val) {
	auto object = info.as_object();
	if (!object) throw std::runtime_error("not an object");

	std::unordered_set< std::string > used; //try all the keys of the object that were used

	used.emplace("FORMAT");
	if (!(object->count("FORMAT") && object->at("FORMAT").as_string() && *object->at("FORMAT").as_string() == "s3d-hm-1")) {
		warn("Loading Halfedge_Mesh from blob without a recognized FORMAT. (Will proceed regardless.)");
	}

	auto read_vector = [&](std::string const &name, auto *vec, std::string const &type){
		used.emplace(name);

		auto f = object->find(name);
		if (f == object->end()) {
			warn("Missing %s when loading Halfedge_Mesh (continuing anyway).", name.c_str());
			return;
		}
		try {
			from_json_base64(f->second, vec, type);
		} catch (std::runtime_error const &e) {
			warn("Failed to load %s while loading Halfedge_Mesh: %s. (Will continue anyway.)", name.c_str(), e.what());
			vec->clear();
		}
	};

	std::vector< uint32_t > halfedge_nexts;
	std::vector< Vec2 > halfedge_corner_uvs;
	std::vector< Vec3 > halfedge_corner_normals;

	read_vector("halfedge_nexts", &halfedge_nexts, "uint32:");
	read_vector("halfedge_corner_uvs", &halfedge_corner_uvs, "vec2:");
	read_vector("halfedge_corner_normals", &halfedge_corner_normals, "vec3:");

	
	std::vector< uint32_t > bone_weight_bones;
	std::vector< float > bone_weight_weights;

	std::vector< uint32_t > vertex_halfedges;
	std::vector< Vec3 > vertex_positions;
	std::vector< uint32_t > vertex_bone_weight_ends; //(begin is previous vertex's end, so not storing)

	read_vector("bone_weight_bones", &bone_weight_bones, "uint32:");
	read_vector("bone_weight_weights", &bone_weight_weights, "float:");
	read_vector("vertex_halfedges", &vertex_halfedges, "uint32:");
	read_vector("vertex_positions", &vertex_positions, "vec3:");
	read_vector("vertex_bone_weight_ends", &vertex_bone_weight_ends, "uint32:");


	std::vector< bool > edge_sharps;

	read_vector("edge_sharps", &edge_sharps, "bool:");


	std::vector< uint32_t > face_halfedges;
	std::vector< bool > face_boundaries;

	read_vector("face_halfedges", &face_halfedges, "uint32:");
	read_vector("face_boundaries", &face_boundaries, "bool:");

	for (auto const &kv : *object) {
		if (!used.count(kv.first)) {
			warn("Ignored unknown member \"%s\" when loading mesh.", kv.first.c_str());
		}
	}

	//------- data is loaded, translate into mesh -------
	Halfedge_Mesh mesh;

	//- - - - - - - - - - -
	//halfedges

	if (halfedge_nexts.size() % 2 != 0) throw std::runtime_error("halfedge without twin");

	#define RESIZE_AND_COMPLAIN(vec, match, vec_name, match_name, fill) \
		if (vec.size() != match.size()) { \
			warn("Have %u " vec_name " for %u " match_name "; resizing.", uint32_t(vec.size()), uint32_t(match.size())); \
			vec.resize(match.size(), fill); \
		}
	
	RESIZE_AND_COMPLAIN(halfedge_corner_uvs, halfedge_nexts, "corner uvs", "halfedges", Vec2(0.0f, 0.0f));
	RESIZE_AND_COMPLAIN(halfedge_corner_normals, halfedge_nexts, "corner normals", "halfedges", Vec3(0.0f, 0.0f, 0.0f));

	//allocate halfedges and get a list of references:
	std::vector< Halfedge_Mesh::HalfedgeRef > halfedges;
	halfedges.reserve(halfedge_nexts.size());
	for (uint32_t i = 0; i < halfedge_nexts.size(); ++i) {
		halfedges.emplace_back(mesh.emplace_halfedge());
	}

	//set halfedge twin pointers and data:
	for (uint32_t i = 0; i < halfedges.size(); ++i) {
		halfedges[i]->twin = halfedges[i ^ 1];
		if (halfedge_nexts[i] >= halfedges.size()) throw std::runtime_error("halfedge with out-of-range next");
		halfedges[i]->next = halfedges[halfedge_nexts[i]];
		halfedges[i]->corner_uv = halfedge_corner_uvs[i];
		halfedges[i]->corner_normal = halfedge_corner_normals[i];
	}

	{ //check that next pointers form a 1-1 mapping:
		//(important so that vertex and face circulation to set pointers terminates)
		std::unordered_set< Halfedge_Mesh::Halfedge * > mentioned;
		for (auto const &h : halfedges) {
			auto ret = mentioned.insert(&*h);
			if (!ret.second) throw std::runtime_error("two halfedges with the same next.");
		}
		assert(mentioned.size() == halfedges.size());
	}

	//- - - - - - - - - - -
	//vertices

	RESIZE_AND_COMPLAIN(vertex_positions, vertex_halfedges, "positions", "vertices", Vec3(0.0f, 0.0f, 0.0f));

	uint32_t last_bone = (vertex_bone_weight_ends.empty() ? 0 : vertex_bone_weight_ends.back());
	RESIZE_AND_COMPLAIN(vertex_bone_weight_ends, vertex_halfedges, "bone weight ranges", "vertices", last_bone);

	if (bone_weight_bones.size() != last_bone) {
		warn("Have %u bones for %u vertex bone weights; resizing.", uint32_t(bone_weight_bones.size()), uint32_t(last_bone));
		bone_weight_bones.resize(last_bone, 0);
	}
	RESIZE_AND_COMPLAIN(bone_weight_weights, bone_weight_bones, "weights", "bones", 0.0f);

	for (uint32_t i = 0; i < vertex_halfedges.size(); ++i) {
		auto v = mesh.emplace_vertex();
		if (vertex_halfedges[i] >= halfedges.size()) throw std::runtime_error("vertex with out-of-range halfedge");
		v->halfedge = halfedges[vertex_halfedges[i]];
		v->position = vertex_positions[i];

		uint32_t begin = (i == 0 ? 0 : vertex_bone_weight_ends[i-1]);
		uint32_t end = vertex_bone_weight_ends[i];
		if (!(begin <= end && end <= bone_weight_bones.size())) {
			warn("Ignoring bone weights for vertex with invalid bone weight range.");
		} else {
			v->bone_weights.reserve(end - begin);
			for (uint32_t w = begin; w < end; ++w) {
				v->bone_weights.emplace_back();
				v->bone_weights.back().bone = bone_weight_bones[w];
				v->bone_weights.back().weight = bone_weight_weights[w];
			}
		}

		//set halfedge -> vertex pointers:
		auto h = v->halfedge;
		do {
			if (h->vertex != mesh.vertices.end()) throw std::runtime_error("two vertices which claim the same halfedge");
			h->vertex = v;
			h = h->twin->next;
		} while (h != v->halfedge);
	}

	//- - - - - - - - - - -
	//edges

	if (edge_sharps.size() != halfedges.size()/2) {
		warn("Have %u sharp flags for %u edges; resizing.", uint32_t(edge_sharps.size()), uint32_t(halfedges.size()/2));
		edge_sharps.resize(halfedges.size()/2, 0);
	}

	for (uint32_t i = 0; i < edge_sharps.size(); ++i) {
		auto e = mesh.emplace_edge();
		e->halfedge = halfedges[i*2];
		e->sharp = edge_sharps[i];

		//this are asserts because at this point it is certain that these are unset:
		assert(e->halfedge->edge == mesh.edges.end());
		assert(e->halfedge->twin->edge == mesh.edges.end());
		e->halfedge->edge = e;
		e->halfedge->twin->edge = e;
	}

	//- - - - - - - - - - -
	//faces

	RESIZE_AND_COMPLAIN(face_boundaries, face_halfedges, "boundary flags", "faces", false);

	for (uint32_t i = 0; i < face_halfedges.size(); ++i) {
		auto f = mesh.emplace_face();
		if (face_halfedges[i] >= halfedges.size()) throw std::runtime_error("face with out-of-range halfedge");
		f->halfedge = halfedges.at(face_halfedges[i]);
		f->boundary = face_boundaries[i];

		auto h = f->halfedge;
		do {
			if (h->face != mesh.faces.end()) throw std::runtime_error("two faces claim the same halfedge");
			h->face = f;
			h = h->next;
		} while (h != f->halfedge);
	}

	#undef RESIZE_AND_COMPLAIN

	//- - - - - - - - - - -
	//finishing up:
	auto note = mesh.validate();
	if (note) {
		warn("Loaded mesh is not valid: %s", note->second.c_str());
	}

	*val = std::move(mesh);
}


//stores a vector of plain-old-data as a base64-encoded blob:
template< typename T >
std::string to_json_base64(std::vector< T > const &data, std::string const &type) {
	static_assert(std::is_standard_layout_v< T >, "should only try to write vectors of standard layout classes as base64");

	unsigned char const *bytes = reinterpret_cast< unsigned char const * >(data.data());
	size_t bytes_size = data.size() * sizeof(T);

	std::string ret;
	size_t expected = 2 + type.size() + (bytes_size * 8 + 5) / 6;
	ret.reserve(expected);

	ret += '"';
	ret += type;
	auto enc = [&ret](uint32_t val) {
		ret += "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[val & 63];
	};

	for (size_t i = 0; i < bytes_size; i += 3) {
		uint32_t to_encode = 0;
		to_encode |= uint32_t(bytes[i]) << 16;
		if (i + 1 < bytes_size) to_encode |= uint32_t(bytes[i+1]) << 8;
		if (i + 2 < bytes_size) to_encode |= uint32_t(bytes[i+2]);
		enc(to_encode >> 18);
		enc(to_encode >> 12);
		if (i + 1 < bytes_size) enc(to_encode >> 6);
		if (i + 2 < bytes_size) enc(to_encode);
	}
	ret += '"';

	if (ret.size() != expected) {
		std::cerr << "NOTE: we mis-estimated the base64 encoding size, somehow (expected " << expected << ", got " << ret.size() << "). Weird." << std::endl;
	}
	return ret;
}


template< typename T >
void from_json_base64(sejp::value const &info, std::vector< T > *data, std::string const &type) {
	static_assert(std::is_standard_layout_v< T >, "should only try to read vectors of standard layout classes as base64");

	auto str_ptr = info.as_string();
	if (!str_ptr) {
		throw std::runtime_error("not a string");
	}
	std::string const &str = *str_ptr;
	if (str.substr(0, type.size()) != type) throw std::runtime_error("does not start with '" + type + "'");

	size_t bytes_size = ((str.size() - type.size()) * 6) / 8;

	if (bytes_size % sizeof(T) != 0) throw std::runtime_error("encoded bytes (" + std::to_string(bytes_size) + ") not a multiple of item size (" + std::to_string(sizeof(T)) + ")");

	std::vector< T > decoded;
	decoded.resize(bytes_size / sizeof(T));

	unsigned char *bytes = reinterpret_cast< unsigned char * >(decoded.data());
	uint32_t o = 0;

	uint32_t buffer = 0;
	uint32_t bits = 0;
	for (size_t i = type.size(); i < str.size(); ++i) {
		buffer <<= 6;
		bits += 6;
		char c = str[i];
		if (c >= 'A' && c <= 'Z') buffer |= uint32_t(c - 'A');
		else if (c >= 'a' && c <= 'z') buffer |= uint32_t(c - 'a') + 26;
		else if (c >= '0' && c <= '9') buffer |= uint32_t(c - '0') + 26 + 26;
		else if (c == '+') buffer |= 10 + 26 + 26;
		else if (c == '/') buffer |= 1 + 10 + 26 + 26;
		else throw std::runtime_error(std::string("invalid character '") + c + "'");
		if (bits >= 8) {
			assert(o < bytes_size);
			bytes[o] = static_cast< unsigned char >(buffer >> (bits - 8));
			++o;
			bits -= 8;
		}
	}
	assert(o == bytes_size);
	*data = std::move(decoded);
}


//special case for bool:
std::string to_json_base64(std::vector< bool > const &data, std::string const &type) {
	std::vector< uint8_t > packed;
	packed.reserve( (data.size() + 7) / 8 );
	uint32_t buffer = 0;
	uint32_t bits = 0;
	auto append_bit = [&](uint8_t val) {
		buffer = (buffer << 1) | val;
		bits += 1;
		if (bits >= 8) {
			packed.emplace_back(uint8_t(buffer & 0xff));
			bits -= 8;
		}
	};
	for (auto b : data) {
		append_bit((b ? 1 : 0));
	}
	//unpacking will trim data until (a) empty or (b) it removes a 1.
	if (packed.empty() && bits == 0) {
		//no padding needed
	} else {
		//make sure there is 1 to remove:
		append_bit(1);
		while (bits != 0) append_bit(0);
	}

	return to_json_base64(packed, type);
}

void from_json_base64(sejp::value const &info, std::vector< bool > *data, std::string const &type) {
	std::vector< uint8_t > packed;
	from_json_base64(info, &packed, type);

	std::vector< bool > unpacked;
	unpacked.reserve(packed.size() * 8);
	for (uint8_t byte : packed) {
		for (uint32_t bit = 7; bit < 8; --bit) {
			unpacked.emplace_back((byte & (1 << bit)) != 0);
		}
	}

	while (!unpacked.empty()) {
		bool back = unpacked.back();
		unpacked.pop_back();
		if (back) break;
	}

	*data = std::move(unpacked);
}




#define DO( T ) \
	template std::string to_json_base64(std::vector< T > const &, std::string const &); \
	template void from_json_base64(sejp::value const &, std::vector< T > *, std::string const &);

DO( uint8_t )
