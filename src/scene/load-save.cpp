
#include "animator.h"
#include "scene.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_set>

#ifdef _MSC_VER
#define BEGIN_PACK __pragma(pack(push, 1))
#define END_PACK __pragma(pack(pop))
#else
#define BEGIN_PACK
#define END_PACK __attribute__((packed))
#endif

//----------
// helpers for saving/loading chunks consisting of arrays of plain-old-data structures
// These work with chunks that look like:
// FFFFBBBBDDD...DDD
//  FFFF: four-byte chunk label
//  BBBB: four-byte count of bytes (little-endian unsigned integer)
//  DD...DDD: BBBB-byte array of data

// helper for saving:
//  fourcc.size() must be 4
//  number of bytes of data (data.size() * sizeof(T)) must fit in uint32_t
template<typename T>
void write(std::ostream& out, const char (&fourcc)[4], std::vector<T> const& data) {
	assert(size_t(uint32_t(data.size() * sizeof(T))) == data.size() * sizeof(T));

	out.write(fourcc, 4);

	uint32_t bytes =  static_cast<uint32_t>(data.size() * sizeof(T));
	out.write(reinterpret_cast<const char*>(&bytes), 4);

	out.write(reinterpret_cast<const char*>(data.data()), bytes);
}

// helper for loading:
//  data must not be null
//  throws if it runs out of data
//  throws if fourcc doesn't match loaded fourcc
//  throws if loaded bytes count isn't a multiple of sizeof(T)
template<typename T> void read(std::istream& in, const char (&fourcc)[4], std::vector<T>* data_) {
	assert(data_);
	auto& data = *data_;
	data.clear();

	struct {
		char fourcc[4];
		uint32_t bytes;
	} header;
	if (!in.read(reinterpret_cast< char * >(&header), sizeof(header))) throw std::runtime_error("Out of bytes reading header of '" + std::string(fourcc,4) + "' chunk.");
	if (std::memcmp(header.fourcc, fourcc, 4) != 0) throw std::runtime_error("Expected '" + std::string(fourcc,4) + "' chunk, but read '" + std::string(header.fourcc,4) + "' chunk.");

	if (header.bytes % sizeof(T) != 0) throw std::runtime_error( "Bytes in '" + std::string(fourcc,4) + "' chunk (" + std::to_string(header.bytes) + ") is not a multiple of type size (" + std::to_string(sizeof(T)) + ").");

	uint32_t count = header.bytes / sizeof(T);

	data.resize(count);

	if (!in.read(reinterpret_cast<char*>(data.data()), sizeof(T) * data.size())) throw std::runtime_error("Out of bytes reading data of '" + std::string(fourcc,4) + "' chunk.");
}

//----------------------
//"plain old data" versions of the elements of the scene:
// (these are what are written/read)

namespace s3ds {

	//scene starts with a header that has a fourcc, count of bytes, and a version:
	constexpr char Header_fourcc[4] = {'s','3','d','s'}; //<-- this is the expected fourcc
	struct Header {
		char fourcc[4];
		uint32_t bytes; //length of the remainder of the s3ds chunk (that is the whole length - the first 8 bytes [the fourcc + this length])
		//^^ this is a "RIFF"-style header, and is designed to make it easy to skip chunks a reader doesn't understand
		uint32_t version; //version is here instead of in fourcc so that it easier to check if a file is an s3d file by looking at the first four bytes.
	};

	//next is the "strings table": an array of characters with fourcc 'str0':
	constexpr char Strings_fourcc[4] = {'s','t','r','0'};
	// (no special structure needed)

	//now texture data:
	constexpr char Texture_Data_fourcc[4] = {'t','x','d','0'};
	//just bytes, no structure needed

	//now textures and the materials that use them:
	constexpr char Textures_fourcc[4] = {'t','e','x','0'};
	BEGIN_PACK struct Texture {
		uint32_t name_begin, name_end;
		uint32_t data_begin, data_end;
		enum : char {
			Constant = 'c', //data is a 16-byte TextureConstantData (see below)
			Image = 'i', //data is TextureImageData (see below) followed by bytes to be passed to HDR_Image::decode()
		} type;
	} END_PACK;
	static_assert(sizeof(Texture) == 4*4+1, "Texture is packed.");

	struct TextureConstantData {
		float r, g, b, scale;
	};
	static_assert(sizeof(TextureConstantData) == 4*4, "TextureConstantData is packed.");

	BEGIN_PACK struct TextureImageData {
		enum : uint8_t {
			Nearest = 0,
			Bilinear = 1,
			Trilinear = 2,
		} interpolation;
	} END_PACK;
	static_assert(sizeof(TextureImageData) == 1, "TextureImageData is packed.");

	constexpr char Materials_fourcc[4] = {'m','a','t','0'};
	BEGIN_PACK struct Material {
		uint32_t name_begin, name_end;
		uint32_t albedo; //albedo is textures[albedo] (for Lambertian only)
		uint32_t reflectance; //reflectance is textures[reflectance] (for Mirror, Glass only)
		uint32_t transmittance; //transmittance is textures[transmittance] (for Refract, Glass only)
		uint32_t emission; //emission is textures[emission] (for Emissive only)
		float ior; //used by Refract, Glass
		enum : char {
			Lambertian = 'l',
			Mirror = 'm',
			Refract = 'r',
			Glass = 'g',
			Emissive = 'e',
		} type;
	} END_PACK;
	static_assert(sizeof(Material) == 7*4+1, "Material is packed.");

	//next are the transforms: an array of s3ds::Transform with fourcc 'xfm0':
	constexpr char Transforms_fourcc[4] = {'x','f','m','0'};
	struct Transform {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t parent; //parent is transforms[parent] (or -1U if no parent)
		float translation[3];
		float rotation[4]; //as a unit-length quaternion, xyzw storage order
		float scale[3];
	};
	static_assert(sizeof(Transform) == 13*4, "Transform is packed.");

	constexpr char Cameras_fourcc[4] = {'c','a','m','0'};
	BEGIN_PACK struct Camera {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		float fov; //vertical fov in degrees
		float aspect; // width / height -- may differ from film size for some reason :-/
		float near_; //near plane distance
		//render settings:
		uint32_t film_width, film_height; //film size (pixels)
		uint32_t film_samples; //samples per pixel in film
		uint32_t film_max_ray_depth; //max ray depth in film
		uint32_t film_sample_pattern; //sampling pattern (rasterizer)
	};
	static_assert(sizeof(Camera) == 10*4, "Camera is packed.");


	//halfedge meshes use a shared list of halfedges, vertices, edges, and faces:
	constexpr char Halfedges_fourcc[4] = {'1','2','e','0'};
	BEGIN_PACK struct Halfedge {
		//uint32_t over; //halfedges are stored in pairs, so over is always (index ^ 1)
		uint32_t next; //index into halfedges list
		float corner_uv[2]; //uv coord at this face corner
		float corner_normal[3]; //shading normal at this face corner
	};
	static_assert(sizeof(Halfedge) == 4*6, "Halfedge is packed.");

	constexpr char Vertices_fourcc[4] = {'v','t','x','0'};
	struct Vertex {
		uint32_t halfedge; //first halfedge around vertex (index into halfedges list)
		float position[3]; //position at this vertex
	};
	static_assert(sizeof(Vertex) == 4*4, "Vertex is packed.");

	constexpr char Edges_fourcc[4] = {'e','d','g','0'};
	BEGIN_PACK struct Edge {
		uint32_t halfedge; //first halfedge on edge (index into halfedges list)
		enum : uint8_t {
			Smooth = 0, //treat this edge as connected when computing shading normals
			Sharp = 1, //treat this edge a break in the surface when computing shading normals
		} sharp_flag;
	} END_PACK;
	static_assert(sizeof(Edge) == 5, "Edge is packed.");

	constexpr char Faces_fourcc[4] = {'f','c','e','0'};
	BEGIN_PACK struct Face {
		uint32_t halfedge; //first halfedge around face (index into halfedges list)
		enum : uint8_t {
			Surface = 0, //regular surface of mesh
			Boundary = 1 //surrounds a hole in the surface
		} boundary_flag;
	} END_PACK;
	static_assert(sizeof(Face) == 5, "Face is packed.");

	constexpr char Halfedge_Meshes_fourcc[4] = {'h','e','m','0'};
	struct Halfedge_Mesh {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t halfedges_begin, halfedges_end;
		uint32_t vertices_begin, vertices_end;
		uint32_t edges_begin, edges_end;
		uint32_t faces_begin, faces_end;
	};
	static_assert(sizeof(Halfedge_Mesh) == 10*4, "Halfedge_Mesh is packed.");

	//skinned meshes have a halfedges, edges, faces pool, just like halfedge meshes.

	//they also have a weights pool (just before their vertices pool):
	constexpr char Weights_fourcc[4] = {'j','w','t','0'};
	struct Weight {
		uint32_t bone; //index into global 'Bones' list
		float weight; //weight of this bone's transform
	};
	static_assert(sizeof(Weight) == 4*2, "Weight is packed.");

	//and a different vertex structure:
	constexpr char Skinned_Vertices_fourcc[4] = {'S','v','x','0'};
	struct Skinned_Vertex {
		uint32_t halfedge; //first halfedge around vertex (index into halfedges list)
		float position[3]; //position at this vertex
		uint32_t weights_begin, weights_end; //joint weights in weights list
	};
	static_assert(sizeof(Skinned_Vertex) == 6*4, "Skinned_Vertex is packed.");

	//and rely on a hierarchy of joints (after their other pools):
	constexpr char Bones_fourcc[4] = {'J','n','t','0'};
	struct Bone {
		//uint32_t name_begin, name_end; //<-- maybe?
		uint32_t parent; //index into overall bones list, or -1U if a root
		float radius;
		float pose[3]; //euler angle XYZ pose (in degrees) relative to parent joint
		float extent[3]; //position of tip in local space
	};
	static_assert(sizeof(Bone) == 8*4, "Bone is packed.");

	//and possibly ik handles
	constexpr char Handles_fourcc[4] = {'H','d','l','0'};
	BEGIN_PACK struct Handle {
		//uint32_t name_begin, name_end; //<-- maybe?
		uint32_t bone; //index into overall bones list, or -1U if a root
		float target[3]; //target position vector
		uint8_t enabled_flag;
	} END_PACK;
	static_assert(sizeof(Handle) == 4*4+1, "Handle is packed.");

	constexpr char Skinned_Meshes_fourcc[4] = {'S','h','m','0'};
	struct Skinned_Mesh {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t halfedges_begin, halfedges_end;
		uint32_t vertices_begin, vertices_end;
		uint32_t edges_begin, edges_end;
		uint32_t faces_begin, faces_end;
		uint32_t bones_begin, bones_end;
		uint32_t handles_begin, handles_end;
		float base[3]; //skeleton base position vector
	};
	static_assert(sizeof(Skinned_Mesh) == 17*4, "Skinned_Mesh is packed.");

	constexpr char Shapes_fourcc[4] = {'s','h','p','0'};
	BEGIN_PACK struct Shape {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		float radius;
		enum : char {
			Sphere = 's',
		} type;
	} END_PACK;
	static_assert(sizeof(Shape) == 3*4+1, "Shape is packed.");

	constexpr char Particles_fourcc[4] = {'p','r','t','0'};
	struct Particle {
		float position[3];
		float velocity[3];
		float age;
	};
	static_assert(sizeof(Particle) == 7*4, "Particle is packed.");

	constexpr char Particle_Systems_fourcc[4] = {'p','r','s','0'};
	struct Particle_System {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t particles_begin, particles_end; //current particles

		float gravity;
		float scale;
		float initial_velocity;
		float spread_angle;
		float lifetime;
		float pps;
		float step_size;
	};
	static_assert(sizeof(Particle_System) == 11*4, "Particle_System is packed.");

	constexpr char Lights_fourcc[4] = {'l','i','t','0'};
	BEGIN_PACK struct Light {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		float color[3]; //spectrum as rgb
		float intensity; //multiplied by spectrum for final light output
		float inner_angle, outer_angle; //start/end of penumbra, degrees (only used for Spot)
		enum : char {
			Point = 'p',
			Directional = 'd',
			Spot = 's',
		} type;
	} END_PACK;
	static_assert(sizeof(Light) == 8*4+1, "Light is packed.");

	constexpr char Environments_fourcc[4] = {'e','n','v','0'};
	BEGIN_PACK struct Environment {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t texture; //texture is textures[texture] or none if texture is -1U
		float intensity;
		enum : char {
			Hemisphere = 'h',
			Sphere = 'o',
		} type;

	} END_PACK;

	static_assert(sizeof(Environment) == 4*4+1, "Environment is packed.");

	// - - - - instances - - - -

	constexpr char Camera_Instances_fourcc[4] = {'I','c','m','0'};
	struct Camera_Instance {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t transform; //index in transforms list
		uint32_t camera; //index in cameras list
	};
	static_assert(sizeof(Camera_Instance) == 4*4, "Camera_Instance is packed.");

	enum Flags : uint32_t {
		FlagsVisible   = 0x01,
		FlagsSimHere   = 0x04, //only makes sense for physics objects

		//draw style for geometry uses bits 0x02 and 0x08 (...because of how things got allocated...)
		FlagsDrawStyleMask = 0x0a,
		FlagsDrawStyleWireframe = 0x02,
		FlagsDrawStyleFlat      = 0x08,
		FlagsDrawStyleSmooth    = 0x0a,
		FlagsDrawStyleCorrect   = 0x00,

		FlagsBlendStyleMask = 0x00a,
		FlagsBlendStyleReplace  = 0x002,
		FlagsBlendStyleAdd      = 0x008,
		FlagsBlendStyleOver     = 0x00a,

		FlagsDepthStyleMask = 0x000a,
		FlagsDepthStyleAlways   = 0x0002,
		FlagsDepthStyleNever    = 0x0008,
		FlagsDepthStyleLess     = 0x000a,
	};

	constexpr char Mesh_Instances_fourcc[4] = {'I','m','e','0'};
	BEGIN_PACK struct Mesh_Instance {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t transform; //index in transforms list
		uint32_t item; //index in meshes
		uint32_t material; //index in materials
		uint32_t flags; //see flags enum above
	} END_PACK;
	static_assert(sizeof(Mesh_Instance) == 6*4, "Mesh_Instance is packed.");

	constexpr char Skinned_Mesh_Instances_fourcc[4] = {'I','s','k','0'};
	using Skinned_Mesh_Instance = Mesh_Instance; //only difference is that 'item' references skinned_meshes

	constexpr char Shape_Instances_fourcc[4] = {'I','s','h','0'};
	using Shape_Instance = Mesh_Instance; //only difference is that 'item' references shapes

	constexpr char Particles_Instances_fourcc[4] = {'I','p','a','0'};
	BEGIN_PACK struct Particles_Instance {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t transform; //index in transforms list
		uint32_t mesh; //index in meshes
		uint32_t material; //index in materials
		uint32_t particles; //index in particles
		uint8_t flags; //see flags enum above
	} END_PACK;
	static_assert(sizeof(Particles_Instance) == 6*4+1, "Particles_Instance is packed.");

	constexpr char Light_Instances_fourcc[4] = {'I','l','i','0'};
	BEGIN_PACK struct Light_Instance {
		uint32_t name_begin, name_end; //name is strings[name_begin,name_end)
		uint32_t transform; //index in transforms list
		uint32_t light; //index in lights
		uint8_t flags; //see flags enum above
	} END_PACK;
	static_assert(sizeof(Light_Instance) == 4*4+1, "Light_Instance is packed.");

	constexpr char Environment_Instances_fourcc[4] = {'I','e','n','0'};
	using Environment_Instance = Light_Instance; //only difference is that 'light' references environments


} //namespace s3ds

namespace s3da {

	//s3da chunk follows s3ds chunk and starts with a header
	constexpr char Header_fourcc[4] = {'s','3','d','a'};
	struct Header {
		char fourcc[4];
		uint32_t bytes;
		uint32_t version; 
	};

	//Resource names and channel names
	constexpr char Strings_fourcc[4] = {'s','t','r','0'};

	//spline data
	constexpr char Spline_Data_fourcc[4] = {'s','p','d','0'};

	//splines that use the data
	constexpr char Splines_fourcc[4] = {'s','p','l','0'};
	BEGIN_PACK struct Spline {
		uint32_t name_begin; //resource name range; name_end = path_begin
		uint32_t path_begin, path_end; //channel name range
		uint32_t data_begin, data_end;
		enum : char {
			Bool = 'b', 
			Float = 'f',
			Vec2 = '2',
			Vec3 = '3',
			Vec4 = '4',
			Quat = 'q',
			Spectrum = 's',
			Mat4 = 'm',
		} type;
	} END_PACK;
	static_assert(sizeof(Spline) == 4*5+1, "Spline is packed.");

	//data stored in each control point, by types
	BEGIN_PACK struct SplineBoolData {
		float time;
		uint8_t value; //true = 1, false = 0
	} END_PACK;
	static_assert(sizeof(SplineBoolData) == 5, "SplineBoolData is packed.");

	struct SplineFloatData {
		float time, value;
	};
	static_assert(sizeof(SplineFloatData) == 8, "SplineFloatData is packed.");

	struct SplineVec2Data {
		float time;
		float value[2];
	};
	static_assert(sizeof(SplineVec2Data) == 4*3, "SplineVec2Data is packed.");

	struct SplineVec3Data {
		float time;
		float value[3];
	};
	static_assert(sizeof(SplineVec3Data) == 4*4, "SplineVec3Data is packed.");

	struct SplineVec4Data {
		float time;
		float value[4];
	};
	static_assert(sizeof(SplineVec4Data) == 4*5, "SplineVec4Data is packed.");

	struct SplineQuatData {
		float time;
		float value[4]; //x, y, z, w
	};
	static_assert(sizeof(SplineQuatData) == 4*5, "SplineQuatData is packed.");

	struct SplineSpectrumData {
		float time;
		float value[3]; //r, g, b
	};
	static_assert(sizeof(SplineSpectrumData) == 4*4, "SplineSpectrumData is packed.");

	struct SplineMat4Data {
		float time;
		float value[16]; //entries in the 4*4 matrix in row-major order
	};
	static_assert(sizeof(SplineMat4Data) == 4*17, "SplineMat4Data is packed.");

} //namespace s3da

//helper:


Scene Scene::load(std::istream& from) {

	//keep track of the # of bytes read:
	auto whence = from.tellg();

	auto file_info = [&]() -> std::string {
		return "[at " + std::to_string(from.tellg()) + "] ";
	};

	Scene scene;

	//starts with a header:
	s3ds::Header header;
	if (!from.read(reinterpret_cast< char * >(&header), sizeof(header))) throw std::runtime_error(file_info() + "Failed to read s3ds header.");

	if (std::memcmp(s3ds::Header_fourcc, header.fourcc, 4) != 0) throw std::runtime_error(file_info() + "Got fourcc '" + std::string(header.fourcc, 4) + "', expected '" + std::string(s3ds::Header_fourcc, 4) + "'.");

	if (header.version > 0) throw std::runtime_error(file_info() + "Version " + std::to_string(header.version) + " is newer than latest supported (0).");

	//keep track of the names used:
	std::unordered_set< std::string > names;
	auto check_name = [&](const char *what, std::string const &name) {
		if (!names.emplace(name).second) throw std::runtime_error(file_info() + std::string(what) + " has duplicated name " + name + ".");
	};

	//helpful for checking ranges:
	#define CHECK_RANGE(Thing, items, begin, end) \
		if ((begin) > (end) || (end) > (items).size()) throw std::runtime_error(file_info() + std::string(Thing) + " has invalid " #items " range[" + std::to_string(begin) + ", " + std::to_string(end) + ") of " + std::to_string((items).size()) + ".")

	//strings chunk:
	std::vector< char > strings;
	read(from, s3ds::Strings_fourcc, &strings);

	auto get_string = [&](std::string const &what, uint32_t begin, uint32_t end) -> std::string {
		if (begin > end || end > strings.size()) throw std::runtime_error(file_info() + "String " + what + " has invalid range [" + std::to_string(begin) + "," + std::to_string(end) + ") of " + std::to_string(strings.size()) + " strings bytes.");
		return std::string(strings.begin() + begin, strings.begin() + end);
	};

	std::vector< std::shared_ptr< Texture > > index_to_texture;
	{ //load textures:
		//texture data chunk:
		std::vector< uint8_t > texture_data;
		read(from, s3ds::Texture_Data_fourcc, &texture_data);
		//actual texture structures:
		std::vector< s3ds::Texture > textures;
		read(from, s3ds::Textures_fourcc, &textures);
		for (auto const &loaded : textures) {
			std::string name = get_string("Texture name", loaded.name_begin, loaded.name_end);
			check_name("Texture", name);
			CHECK_RANGE("Texture", texture_data, loaded.data_begin, loaded.data_end);

			std::shared_ptr< Texture > texture;
			if (loaded.type == s3ds::Texture::Constant) {
				s3ds::TextureConstantData tcd;
				if (loaded.data_end - loaded.data_begin != sizeof(tcd)) throw std::runtime_error(file_info() + "Texture with constant color has " + std::to_string(loaded.data_end-loaded.data_begin) + " bytes of data; expected " + std::to_string(sizeof(tcd)) + ".");
				memcpy(&tcd, &texture_data[loaded.data_begin], loaded.data_end - loaded.data_begin);
				Textures::Constant constant;
				constant.color.r = tcd.r;
				constant.color.g = tcd.g;
				constant.color.b = tcd.b;
				constant.scale = tcd.scale;
				texture = std::make_shared< Texture >(constant);
			} else if (loaded.type == s3ds::Texture::Image) {
				s3ds::TextureImageData tid;
				if (loaded.data_begin + sizeof(tid) > loaded.data_end) throw std::runtime_error(file_info() + "Texture with image has " + std::to_string(loaded.data_end-loaded.data_begin) + " bytes of data; expected at least " + std::to_string(sizeof(tid)) + ".");
				memcpy(&tid, &texture_data[loaded.data_begin], sizeof(tid));

				Textures::Image image;

				//interpolation:
				if (tid.interpolation == s3ds::TextureImageData::Nearest) {
					image.sampler = Textures::Image::Sampler::nearest;
				} else if (tid.interpolation == s3ds::TextureImageData::Bilinear) {
					image.sampler = Textures::Image::Sampler::bilinear;
				} else if (tid.interpolation == s3ds::TextureImageData::Trilinear) {
					image.sampler = Textures::Image::Sampler::trilinear;
				} else {
					throw std::runtime_error(file_info() + "Texture with image has unknown interpolation type '" + std::to_string(uint32_t(tid.interpolation)) + "'.");
				}

				//image data:
				try {
					image.image = HDR_Image::decode(&texture_data[loaded.data_begin + sizeof(tid)], loaded.data_end - (loaded.data_begin + sizeof(tid)));
				} catch (std::exception const &e) {
					throw std::runtime_error(file_info() + "Texture with image data that failed to decode: " + std::string(e.what()));
				}

				//generate mipmap if required by sampler:
				image.update_mipmap();

				texture = std::make_shared< Texture >(std::move(image));
			} else {
				throw std::runtime_error(file_info() + "Texture has unknown type '" + std::string(reinterpret_cast< const char * >(&loaded.type), 1) + "'.");
			}

			scene.textures.emplace(name, texture);
			index_to_texture.emplace_back(texture);
		}
	}

	std::vector< std::shared_ptr< Material > > index_to_material;
	{ //load materials:
		std::vector< s3ds::Material > materials;
		read(from, s3ds::Materials_fourcc, &materials);
		for (auto const &loaded : materials) {
			std::string name = get_string("Material name", loaded.name_begin, loaded.name_end);
			check_name("Material", name);

			std::shared_ptr< Material > material;
			if (loaded.type == s3ds::Material::Lambertian) {
				Materials::Lambertian lambertian;
				if (loaded.albedo != static_cast<uint32_t>(-1)) {
					if (loaded.albedo >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range albedo texture.");
					lambertian.albedo = index_to_texture[loaded.albedo];
				}
				material = std::make_shared< Material >(lambertian);
			} else if (loaded.type == s3ds::Material::Mirror) {
				Materials::Mirror mirror;
				if (loaded.reflectance != static_cast<uint32_t>(-1)) {
					if (loaded.reflectance >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range reflectance texture.");
					mirror.reflectance = index_to_texture[loaded.reflectance];
				}
				material = std::make_shared< Material >(mirror);
			} else if (loaded.type == s3ds::Material::Refract) {
				Materials::Refract refract;
				if (loaded.transmittance != static_cast<uint32_t>(-1)) {
					if (loaded.transmittance >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range transmittance texture.");
					refract.transmittance = index_to_texture[loaded.transmittance];
				}
				refract.ior = loaded.ior;
				material = std::make_shared< Material >(refract);
			} else if (loaded.type == s3ds::Material::Glass) {
				Materials::Glass glass;
				if (loaded.reflectance != static_cast<uint32_t>(-1)) {
					if (loaded.reflectance >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range reflectance texture.");
					glass.reflectance = index_to_texture[loaded.reflectance];
				}
				if (loaded.transmittance != static_cast<uint32_t>(-1)) {
					if (loaded.transmittance >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range transmittance texture.");
					glass.transmittance = index_to_texture[loaded.transmittance];
				}
				glass.ior = loaded.ior;
				material = std::make_shared< Material >(glass);
			} else if (loaded.type == s3ds::Material::Emissive) {
				Materials::Emissive emissive;
				if (loaded.emission != static_cast<uint32_t>(-1)) {
					if (loaded.emission >= index_to_texture.size()) throw std::runtime_error(file_info() + "Material has out-of-range emission texture.");
					emissive.emissive = index_to_texture[loaded.emission];
				}
				material = std::make_shared< Material >(emissive);
			} else {
				throw std::runtime_error(file_info() + "Material has unknown type '" + std::string(reinterpret_cast< const char * >(&loaded.type), 1) + "'.");
			}

			scene.materials.emplace(name, material);
			index_to_material.emplace_back(material);
		}
	}

	std::vector< std::shared_ptr< Transform > > index_to_transform;
	{ //load transforms:
		std::vector< s3ds::Transform > transforms;
		read(from, s3ds::Transforms_fourcc, &transforms);

		index_to_transform.reserve(transforms.size());
		for (auto const &loaded : transforms) {
			std::string name = get_string("Transform name", loaded.name_begin, loaded.name_end);
			check_name("Transform", name);

			std::shared_ptr< Transform > transform = std::make_shared< Transform >();
			if (loaded.parent != static_cast<uint32_t>(-1)) {
				if (loaded.parent >= index_to_transform.size()) throw std::runtime_error(file_info() + "Transforms list is not topologically sorted.");
				transform->parent = index_to_transform[loaded.parent];
			}
			transform->translation = Vec3(loaded.translation[0], loaded.translation[1], loaded.translation[2]);
			transform->rotation = Quat::xyzw(loaded.rotation[3], loaded.rotation[0], loaded.rotation[1], loaded.rotation[2]);
			transform->scale = Vec3(loaded.scale[0], loaded.scale[1], loaded.scale[2]);

			scene.transforms.emplace(name, transform);
			index_to_transform.emplace_back(transform);
		}
	}

	std::vector< std::shared_ptr< Camera > > index_to_camera;
	{ //load cameras:
		std::vector< s3ds::Camera > cameras;
		read(from, s3ds::Cameras_fourcc, &cameras);

		index_to_camera.reserve(cameras.size());
		for (auto const &loaded : cameras) {
			std::string name = get_string("Camera name", loaded.name_begin, loaded.name_end);
			check_name("Camera", name);

			std::shared_ptr< Camera > camera = std::make_shared< Camera >();

			camera->vertical_fov = loaded.fov;
			camera->aspect_ratio = loaded.aspect;
			camera->near_plane = loaded.near_;
			camera->film.width = loaded.film_width;
			camera->film.height = loaded.film_height;
			camera->film.samples = loaded.film_samples;
			camera->film.max_ray_depth = loaded.film_max_ray_depth;
			camera->film.sample_pattern = loaded.film_sample_pattern;

			scene.cameras.emplace(name, camera);
			index_to_camera.emplace_back(camera);
		}
	}

	//mesh loading and skinned mesh loading share a lot of code, so use a common helper function:
	auto load_mesh = [&](
		const char *Thing,
		std::vector< s3ds::Halfedge > const & halfedges,
		auto const & vertices,
		std::vector< s3ds::Edge > const & edges,
		std::vector< s3ds::Face > const & faces,
		auto const &loaded,
		Halfedge_Mesh *mesh,
		auto const &set_extra_vertex_data) {
		assert(mesh);

		// -- halfedges --

		CHECK_RANGE(Thing, halfedges, loaded.halfedges_begin, loaded.halfedges_end);
		if (loaded.halfedges_begin % 2 != 0 || loaded.halfedges_end % 2 != 0) throw std::runtime_error(file_info() + std::string(Thing) + " does not reference a fully-twinned set of halfedges.");

		std::vector< Halfedge_Mesh::HalfedgeRef > halfedge_refs;
		halfedge_refs.reserve(loaded.halfedges_end - loaded.halfedges_begin);

		//allocate halfedges and set data:
		for (uint32_t i = loaded.halfedges_begin; i != loaded.halfedges_end; ++i) {
			s3ds::Halfedge const &he = halfedges[i];
			Halfedge_Mesh::HalfedgeRef halfedge = mesh->emplace_halfedge();
			halfedge->corner_uv = Vec2(he.corner_uv[0], he.corner_uv[1]);
			halfedge->corner_normal = Vec3(he.corner_normal[0], he.corner_normal[1], he.corner_normal[2]);
			halfedge_refs.emplace_back(halfedge);
		}

		//set halfedge next and twin pointers:
		for (uint32_t i = loaded.halfedges_begin; i != loaded.halfedges_end; ++i) {
			uint32_t li = i - loaded.halfedges_begin; //local index
			halfedge_refs[li]->twin = halfedge_refs[li^1]; //twin is always the even/odd pairing
			if (halfedges[i].next < loaded.halfedges_begin || halfedges[i].next >= loaded.halfedges_end) throw std::runtime_error(file_info() + std::string(Thing) + " has a halfedge with an out-of-range next pointer -- next is " + std::to_string(halfedges[i].next) + " but loaded range is [" + std::to_string(loaded.halfedges_begin) + "," + std::to_string(loaded.halfedges_end) +").");
			halfedge_refs[li]->next = halfedge_refs[halfedges[i].next - loaded.halfedges_begin]; //next as per index
		}

		{ //check that next pointers form a 1-1 mapping:
			//(important so that vertex and face circulation to set pointers terminates)
			std::unordered_set< Halfedge_Mesh::Halfedge * > mentioned;
			for (auto const &h : halfedge_refs) {
				auto ret = mentioned.insert(&*h);
				if (!ret.second) throw std::runtime_error(file_info() + std::string(Thing) + " has two halfedges with the same next.");
			}
			assert(mentioned.size() == halfedge_refs.size());
		}

		// -- vertices --

		//allocate vertices and set data, pointers:
		CHECK_RANGE("Halfedge_Mesh", vertices, loaded.vertices_begin, loaded.vertices_end);
		for (uint32_t i = loaded.vertices_begin; i != loaded.vertices_end; ++i) {
			auto const &lv = vertices[i];
			Halfedge_Mesh::VertexRef vertex = mesh->emplace_vertex();
			if (lv.halfedge < loaded.halfedges_begin || lv.halfedge >= loaded.halfedges_end) throw std::runtime_error(file_info() + std::string(Thing) + " has a vertex with an out-of-range halfedge pointer.");
			vertex->halfedge = halfedge_refs[lv.halfedge - loaded.halfedges_begin];
			vertex->position = Vec3(lv.position[0], lv.position[1], lv.position[2]);
			set_extra_vertex_data(lv, vertex);

			//circulate and set all vertex pointers:
			Halfedge_Mesh::HalfedgeRef h = vertex->halfedge;
			do {
				 if (h->vertex != mesh->vertices.end()) throw std::runtime_error(file_info() + std::string(Thing) + " has two vertices that claim the same halfedge.");
				h->vertex = vertex;
				h = h->twin->next;
			} while (h != vertex->halfedge);
		}

		// -- edges --

		//allocate edges and set data, pointers:
		CHECK_RANGE("Halfedge_Mesh", edges, loaded.edges_begin, loaded.edges_end);
		for (uint32_t i = loaded.edges_begin; i != loaded.edges_end; ++i) {
			s3ds::Edge const &le = edges[i];
			Halfedge_Mesh::EdgeRef edge = mesh->emplace_edge();
			if (le.halfedge < loaded.halfedges_begin || le.halfedge >= loaded.halfedges_end) throw std::runtime_error(file_info() + std::string(Thing) + " has an edge with an out-of-range halfedge pointer.");
			edge->halfedge = halfedge_refs[le.halfedge - loaded.halfedges_begin];
			edge->sharp = (le.sharp_flag == s3ds::Edge::Sharp);

			//circulate and set all edge pointers:
			// (yes, it's not much of a circulation but writing it this way keeps things consistent)
			Halfedge_Mesh::HalfedgeRef h = edge->halfedge;
			do {
				if (h->edge != mesh->edges.end()) throw std::runtime_error(file_info() + std::string(Thing) + " has two edges that claim the same halfedge.");
				h->edge = edge;
				h = h->twin;
			} while (h != edge->halfedge);
		}

		// -- faces --
		//allocate faces and set data, pointers:
		CHECK_RANGE("Halfedge_Mesh", faces, loaded.faces_begin, loaded.faces_end);
		for (uint32_t i = loaded.faces_begin; i != loaded.faces_end; ++i) {
			s3ds::Face const &lf = faces[i];
			Halfedge_Mesh::FaceRef face = mesh->emplace_face();
			if (lf.halfedge < loaded.halfedges_begin || lf.halfedge >= loaded.halfedges_end) throw std::runtime_error(file_info() + std::string(Thing) + " has a face with an out-of-range halfedge pointer.");
			face->halfedge = halfedge_refs[lf.halfedge - loaded.halfedges_begin];
			face->boundary = (lf.boundary_flag == s3ds::Face::Boundary);

			//circulate and set all face pointers:
			Halfedge_Mesh::HalfedgeRef h = face->halfedge;
			do {
				if (h->face != mesh->faces.end()) throw std::runtime_error(file_info() + std::string(Thing) + " has two faces that claim the same halfedge.");
				h->face = face;
				h = h->next;
			} while (h != face->halfedge);
		}

		//TODO: could check for validity; all pointers set
	};

	std::vector< std::shared_ptr< Halfedge_Mesh > > index_to_mesh;
	{ //load [halfedge] meshes:
		//halfedges, vertices, edges, faces pools for meshes:
		std::vector< s3ds::Halfedge > halfedges;
		read(from, s3ds::Halfedges_fourcc, &halfedges);
		std::vector< s3ds::Vertex > vertices;
		read(from, s3ds::Vertices_fourcc, &vertices);
		std::vector< s3ds::Edge > edges;
		read(from, s3ds::Edges_fourcc, &edges);
		std::vector< s3ds::Face > faces;
		read(from, s3ds::Faces_fourcc, &faces);

		//the meshes:
		std::vector< s3ds::Halfedge_Mesh > halfedge_meshes;
		read(from, s3ds::Halfedge_Meshes_fourcc, &halfedge_meshes);

		for (auto const &loaded : halfedge_meshes) {
			std::string name = get_string("Halfedge_Mesh name", loaded.name_begin, loaded.name_end);
			check_name("Halfedge_Mesh", name);

			std::shared_ptr< Halfedge_Mesh > mesh = std::make_shared< Halfedge_Mesh >();

			load_mesh("Halfedge_Mesh", halfedges, vertices, edges, faces, loaded, mesh.get(), [](s3ds::Vertex const &, Halfedge_Mesh::VertexRef const &){ /* no extra data to set */ });

			scene.meshes.emplace(name, mesh);
			index_to_mesh.emplace_back(mesh);
		}
	}

	std::vector< std::shared_ptr< Skinned_Mesh > > index_to_skinned_mesh;
	{ //load [skinned] meshes:
		//halfedges, weights, vertices, edges, faces, bones pools for skinned meshes:
		std::vector< s3ds::Halfedge > halfedges;
		read(from, s3ds::Halfedges_fourcc, &halfedges);
		std::vector< s3ds::Weight > weights;
		read(from, s3ds::Weights_fourcc, &weights);
		std::vector< s3ds::Skinned_Vertex > vertices;
		read(from, s3ds::Skinned_Vertices_fourcc, &vertices);
		std::vector< s3ds::Edge > edges;
		read(from, s3ds::Edges_fourcc, &edges);
		std::vector< s3ds::Face > faces;
		read(from, s3ds::Faces_fourcc, &faces);
		std::vector< s3ds::Bone > bones;
		read(from, s3ds::Bones_fourcc, &bones);
		std::vector< s3ds::Handle > handles;
 		read(from, s3ds::Handles_fourcc, &handles);

		//the meshes:
		std::vector< s3ds::Skinned_Mesh > skinned_meshes;
		read(from, s3ds::Skinned_Meshes_fourcc, &skinned_meshes);

		for (auto const &loaded : skinned_meshes) {
			std::string name = get_string("Skinned_Mesh name", loaded.name_begin, loaded.name_end);
			check_name("Skinned_Mesh", name);

			std::shared_ptr< Skinned_Mesh > skinned_mesh = std::make_shared< Skinned_Mesh >();

			auto set_extra_vertex_data = [&weights,&file_info,&loaded](s3ds::Skinned_Vertex const &lv, Halfedge_Mesh::VertexRef const &vertex){
				CHECK_RANGE("Skinned_Vertex", weights, lv.weights_begin, lv.weights_end);
				vertex->bone_weights.reserve(lv.weights_end - lv.weights_begin);
				for (uint32_t i = lv.weights_begin; i < lv.weights_end; ++i) {
					if (weights[i].bone < loaded.bones_begin || weights[i].bone >= loaded.bones_end) throw std::runtime_error(file_info() + "Weight references out-of-range bone.");
					vertex->bone_weights.emplace_back(Halfedge_Mesh::Vertex::Bone_Weight{weights[i].bone - loaded.bones_begin, weights[i].weight});
				}
			};

			//the halfedge mesh:

			load_mesh("Skinned_Mesh", halfedges, vertices, edges, faces, loaded, &skinned_mesh->mesh, set_extra_vertex_data);

			//the bones:
			CHECK_RANGE("Skinned_Mesh", bones, loaded.bones_begin, loaded.bones_end);
			for (uint32_t i = loaded.bones_begin; i != loaded.bones_end; ++i) {
				s3ds::Bone const &lb = bones[i];
				Skeleton::Bone bone;
				bone.extent = Vec3(lb.extent[0], lb.extent[1], lb.extent[2]);
				bone.roll = 0.0f; //not saved :-/
				bone.pose = Vec3(lb.pose[0], lb.pose[1], lb.pose[2]);
				bone.radius = lb.radius;
				bone.channel_id = i - loaded.bones_begin; //not saved (!!)
				if (lb.parent == static_cast<uint32_t>(-1)) {
					bone.parent = -1U;
				} else {
					if (lb.parent < loaded.bones_begin || lb.parent >= loaded.bones_end) throw std::runtime_error(file_info() + "Bone's parent isn't in the same skeleton.");
					uint32_t index = lb.parent - loaded.bones_begin;
					if (index >= i - loaded.bones_begin) throw std::runtime_error(file_info() + "Bone is stored before parent.");
					bone.parent = index;
				}
				skinned_mesh->skeleton.bones.emplace_back(bone);
			}

			//the handles:
 			CHECK_RANGE("Skinned_Mesh", handles, loaded.handles_begin, loaded.handles_end);
 			for (uint32_t i = loaded.handles_begin; i != loaded.handles_end; ++i) {
 				s3ds::Handle const &lh = handles[i];
 				Skeleton::Handle handle;
 				if (lh.bone < loaded.bones_begin || lh.bone >= loaded.bones_end) throw std::runtime_error(file_info() + "IK handle's bone isn't in the same skeleton.");
 				handle.bone = lh.bone - loaded.bones_begin;
 				handle.target = Vec3(lh.target[0], lh.target[1], lh.target[2]);
 				handle.enabled = (lh.enabled_flag != 0);
				handle.channel_id = i - loaded.handles_begin; //not saved (!!)
 				skinned_mesh->skeleton.handles.emplace_back(handle);
 			}

			skinned_mesh->skeleton.base.x = loaded.base[0];
			skinned_mesh->skeleton.base.y = loaded.base[1];
			skinned_mesh->skeleton.base.z = loaded.base[2];

			//base_offset is not stored(!)
 
			scene.skinned_meshes.emplace(name, skinned_mesh);
			index_to_skinned_mesh.emplace_back(skinned_mesh);
		}

	}

	std::vector< std::shared_ptr< Shape > > index_to_shape;
	{ //load shapes:
		std::vector< s3ds::Shape > shapes;
		read(from, s3ds::Shapes_fourcc, &shapes);
		for (auto const &loaded : shapes) {
			std::string name = get_string("Shape name", loaded.name_begin, loaded.name_end);
			check_name("Shape", name);

			std::shared_ptr< Shape > shape;
			if (loaded.type == s3ds::Shape::Sphere) {
				shape = std::make_shared< Shape >(Shapes::Sphere(loaded.radius));
			} else {
				throw std::runtime_error(file_info() + "Shape type '" + std::string(reinterpret_cast< const char * >(&loaded.type), 1) + "' not recognized.");
			}

			scene.shapes.emplace(name, shape);
			index_to_shape.emplace_back(shape);
		}
	}

	std::vector< std::shared_ptr< Particles > > index_to_particles;
	{ //load particle systems:
		std::vector< s3ds::Particle > particles;
		read(from, s3ds::Particles_fourcc, &particles);

		std::vector< s3ds::Particle_System > particle_systems;
		read(from, s3ds::Particle_Systems_fourcc, &particle_systems);

		for (auto const &loaded : particle_systems) {
			std::string name = get_string("Particle_System name", loaded.name_begin, loaded.name_end);
			check_name("Particle_System", name);

			std::shared_ptr< Particles > particle_system = std::make_shared< Particles >();
			particle_system->gravity = Vec3(0.0f, -loaded.gravity, 0.0f);
			particle_system->radius = loaded.scale;
			particle_system->initial_velocity = loaded.initial_velocity;
			particle_system->spread_angle = loaded.spread_angle;
			particle_system->lifetime = loaded.lifetime;
			particle_system->rate = loaded.pps;
			particle_system->seed = 0x31415926;
			particle_system->step_size = loaded.step_size;

			CHECK_RANGE("Particles", particles, loaded.particles_begin, loaded.particles_end);
			particle_system->particles.reserve(loaded.particles_end - loaded.particles_begin);
			for (uint32_t i = loaded.particles_begin; i != loaded.particles_end; ++i) {
				s3ds::Particle const &lp = particles[i];
				Particles::Particle particle;
				particle.position = Vec3(lp.position[0], lp.position[1], lp.position[2]);
				particle.velocity = Vec3(lp.velocity[0], lp.velocity[1], lp.velocity[2]);
				particle.age = lp.age;
				particle_system->particles.emplace_back(particle);
			}

			scene.particles.emplace(name, particle_system);
			index_to_particles.emplace_back(particle_system);
		}

	}

	std::vector< std::shared_ptr< Delta_Light > > index_to_delta_light;
	{ //load lights:
		std::vector< s3ds::Light > lights;
		read(from, s3ds::Lights_fourcc, &lights);

		for (auto const &loaded : lights) {
			std::string name = get_string("Light name", loaded.name_begin, loaded.name_end);
			check_name("Light", name);

			std::shared_ptr< Delta_Light > delta_light = std::make_shared< Delta_Light >();

			if (loaded.type == s3ds::Light::Point) {
				Delta_Lights::Point point;
				point.color = Spectrum(loaded.color[0], loaded.color[1], loaded.color[2]);
				point.intensity = loaded.intensity;
				delta_light->light = point;
			} else if (loaded.type == s3ds::Light::Directional) {
				Delta_Lights::Directional directional;
				directional.color = Spectrum(loaded.color[0], loaded.color[1], loaded.color[2]);
				directional.intensity = loaded.intensity;
				delta_light->light = directional;
			} else if (loaded.type == s3ds::Light::Spot) {
				Delta_Lights::Spot spot;
				spot.color = Spectrum(loaded.color[0], loaded.color[1], loaded.color[2]);
				spot.intensity = loaded.intensity;
				spot.inner_angle = loaded.inner_angle;
				spot.outer_angle = loaded.outer_angle;
				delta_light->light = spot;
			} else {
				throw std::runtime_error(file_info() + "Light with unrecognized type '" + std::string(reinterpret_cast< char const * >(&loaded.type), 1) + "'.");
			}

			scene.delta_lights.emplace(name, delta_light);
			index_to_delta_light.emplace_back(delta_light);
		}
	}

	std::vector< std::shared_ptr< Environment_Light > > index_to_env_light;
	{ //load environment lights:
		std::vector< s3ds::Environment > environments;
		read(from, s3ds::Environments_fourcc, &environments);

		for (auto const &loaded : environments) {
			std::string name = get_string("Environment name", loaded.name_begin, loaded.name_end);
			check_name("Environment", name);

			std::shared_ptr< Environment_Light > env_light = std::make_shared< Environment_Light >();

			if (loaded.type == s3ds::Environment::Hemisphere) {
				Environment_Lights::Hemisphere hemi;
				hemi.intensity = loaded.intensity;
				if (loaded.texture != static_cast<uint32_t>(-1)) {
					if (loaded.texture >= index_to_texture.size()) throw std::runtime_error(file_info() + "Environment with out-of-range texture.");
					hemi.radiance = index_to_texture[loaded.texture];
				}
				env_light->light = hemi;
			} else if (loaded.type == s3ds::Environment::Sphere) {
				Environment_Lights::Sphere sphere;
				sphere.intensity = loaded.intensity;
				if (loaded.texture != static_cast<uint32_t>(-1)) {
					if (loaded.texture >= index_to_texture.size()) throw std::runtime_error(file_info() + "Environment with out-of-range texture.");
					sphere.radiance = index_to_texture[loaded.texture];
				}
				env_light->light = sphere;
			} else {
				throw std::runtime_error(file_info() + "Environment with unrecognized type '" + std::string(reinterpret_cast< char const * >(&loaded.type), 1) + "'.");
			}

			scene.env_lights.emplace(name, env_light);
			index_to_env_light.emplace_back(env_light);
		}

	}

	// - - - - instances - - - -

	{ //camera
		std::vector< s3ds::Camera_Instance > camera_instances;
		read(from, s3ds::Camera_Instances_fourcc, &camera_instances);

		for (auto const &loaded : camera_instances) {
			std::string name = get_string("Camera_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Camera_Instance", name);

			std::shared_ptr< Instance::Camera > instance = std::make_shared< Instance::Camera >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Camera_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.camera != static_cast<uint32_t>(-1)) {
				if (loaded.camera >= index_to_camera.size()) throw std::runtime_error(file_info() + "Camera_Instance with out-of-range camera.");
				instance->camera = index_to_camera[loaded.camera];
			}

			scene.instances.cameras.emplace(name, instance);
		}
	}

	auto flags_to_drawstyle = [&](std::string const &what, uint32_t flags) -> DrawStyle {
		if      ((flags & s3ds::FlagsDrawStyleMask) == s3ds::FlagsDrawStyleWireframe) return DrawStyle::Wireframe;
		else if ((flags & s3ds::FlagsDrawStyleMask) == s3ds::FlagsDrawStyleFlat     ) return DrawStyle::Flat;
		else if ((flags & s3ds::FlagsDrawStyleMask) == s3ds::FlagsDrawStyleSmooth   ) return DrawStyle::Smooth;
		else if ((flags & s3ds::FlagsDrawStyleMask) == s3ds::FlagsDrawStyleCorrect  ) return DrawStyle::Correct;
		else throw std::runtime_error(file_info() + what + " with unknown draw style.");
	};
	auto flags_to_blendstyle = [&](std::string const &what, uint32_t flags) -> BlendStyle {
		if      ((flags & s3ds::FlagsBlendStyleMask) == s3ds::FlagsBlendStyleReplace) return BlendStyle::Replace;
		else if ((flags & s3ds::FlagsBlendStyleMask) == s3ds::FlagsBlendStyleAdd    ) return BlendStyle::Add;
		else if ((flags & s3ds::FlagsBlendStyleMask) == s3ds::FlagsBlendStyleOver   ) return BlendStyle::Over;
		else throw std::runtime_error(file_info() + what + " with unknown blend style.");
	};
	auto flags_to_depthstyle = [&](std::string const &what, uint32_t flags) -> DepthStyle {
		if      ((flags & s3ds::FlagsDepthStyleMask) == s3ds::FlagsDepthStyleAlways ) return DepthStyle::Always;
		else if ((flags & s3ds::FlagsDepthStyleMask) == s3ds::FlagsDepthStyleNever  ) return DepthStyle::Never;
		else if ((flags & s3ds::FlagsDepthStyleMask) == s3ds::FlagsDepthStyleLess   ) return DepthStyle::Less;
		else throw std::runtime_error(file_info() + what + " with unknown depth style.");
	};

	{ //mesh
		std::vector< s3ds::Mesh_Instance > mesh_instances;
		read(from, s3ds::Mesh_Instances_fourcc, &mesh_instances);

		for (auto const &loaded : mesh_instances) {
			std::string name = get_string("Mesh_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Mesh_Instance", name);

			std::shared_ptr< Instance::Mesh > instance = std::make_shared< Instance::Mesh >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) {
					std::cerr << file_info() << "Mesh_Instance '" << name << "' with out-of-range transform." << std::endl; //DEBUG
					//throw std::runtime_error(file_info() + "Mesh_Instance with out-of-range transform.");
				} else {
					instance->transform = index_to_transform[loaded.transform];
				}
			}
			if (loaded.item != static_cast<uint32_t>(-1)) {
				if (loaded.item >= index_to_mesh.size()) throw std::runtime_error(file_info() + "Mesh_Instance with out-of-range mesh.");
				instance->mesh = index_to_mesh[loaded.item];
			}
			if (loaded.material != static_cast<uint32_t>(-1)) {
				if (loaded.material >= index_to_material.size()) throw std::runtime_error(file_info() + "Mesh_Instance with out-of-range material.");
				instance->material = index_to_material[loaded.material];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);
			instance->settings.draw_style = flags_to_drawstyle("Mesh_Instance", loaded.flags);
			instance->settings.blend_style = flags_to_blendstyle("Mesh_Instance", loaded.flags);
			instance->settings.depth_style = flags_to_depthstyle("Mesh_Instance", loaded.flags);

			scene.instances.meshes.emplace(name, instance);
		}
	}

	{ //skinned mesh
		std::vector< s3ds::Skinned_Mesh_Instance > skinned_mesh_instances;
		read(from, s3ds::Skinned_Mesh_Instances_fourcc, &skinned_mesh_instances);

		for (auto const &loaded : skinned_mesh_instances) {
			std::string name = get_string("Skinned_Mesh_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Skinned_Mesh_Instance", name);

			std::shared_ptr< Instance::Skinned_Mesh > instance = std::make_shared< Instance::Skinned_Mesh >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Skinned_Mesh_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.item != static_cast<uint32_t>(-1)) {
				if (loaded.item >= index_to_skinned_mesh.size()) throw std::runtime_error(file_info() + "Skinned_Mesh_Instance with out-of-range skinned mesh.");
				instance->mesh = index_to_skinned_mesh[loaded.item];
			}
			if (loaded.material != static_cast<uint32_t>(-1)) {
				if (loaded.material >= index_to_material.size()) throw std::runtime_error(file_info() + "Skinned_Mesh_Instance with out-of-range material.");
				instance->material = index_to_material[loaded.material];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);
			instance->settings.draw_style = flags_to_drawstyle("Skinned_Mesh_Instance", loaded.flags);
			instance->settings.blend_style = flags_to_blendstyle("Skinned_Mesh_Instance", loaded.flags);
			instance->settings.depth_style = flags_to_depthstyle("Skinned_Mesh_Instance", loaded.flags);

			scene.instances.skinned_meshes.emplace(name, instance);
		}
	}

	{ //shape
		std::vector< s3ds::Shape_Instance > shape_instances;
		read(from, s3ds::Shape_Instances_fourcc, &shape_instances);

		for (auto const &loaded : shape_instances) {
			std::string name = get_string("Shape_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Shape_Instance", name);

			std::shared_ptr< Instance::Shape > instance = std::make_shared< Instance::Shape >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Shape_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.item != static_cast<uint32_t>(-1)) {
				if (loaded.item >= index_to_shape.size()) throw std::runtime_error(file_info() + "Shape_Instance with out-of-range shape.");
				instance->shape = index_to_shape[loaded.item];
			}
			if (loaded.material != static_cast<uint32_t>(-1)) {
				if (loaded.material >= index_to_material.size()) throw std::runtime_error(file_info() + "Shape_Instance with out-of-range material.");
				instance->material = index_to_material[loaded.material];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);
			instance->settings.draw_style = flags_to_drawstyle("Shape_Instance", loaded.flags);
			instance->settings.blend_style = flags_to_blendstyle("Shape_Instance", loaded.flags);
			instance->settings.depth_style = flags_to_depthstyle("Shape_Instance", loaded.flags);

			scene.instances.shapes.emplace(name, instance);
		}
	}

	{ //particles
		std::vector< s3ds::Particles_Instance > particles_instances;
		read(from, s3ds::Particles_Instances_fourcc, &particles_instances);

		for (auto const &loaded : particles_instances) {
			std::string name = get_string("Particles_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Particles_Instance", name);

			std::shared_ptr< Instance::Particles > instance = std::make_shared< Instance::Particles >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Particles_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.mesh != static_cast<uint32_t>(-1)) {
				if (loaded.mesh >= index_to_mesh.size()) throw std::runtime_error(file_info() + "Particles_Instance with out-of-range mesh.");
				instance->mesh = index_to_mesh[loaded.mesh];
			}
			if (loaded.material != static_cast<uint32_t>(-1)) {
				if (loaded.material >= index_to_material.size()) throw std::runtime_error(file_info() + "Particles_Instance with out-of-range material.");
				instance->material = index_to_material[loaded.material];
			}
			if (loaded.particles != static_cast<uint32_t>(-1)) {
				if (loaded.particles >= index_to_particles.size()) throw std::runtime_error(file_info() + "Particles_Instance with out-of-range particles.");
				instance->particles = index_to_particles[loaded.particles];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);
			instance->settings.wireframe = ((loaded.flags & s3ds::FlagsDrawStyleMask) == s3ds::FlagsDrawStyleWireframe);
			instance->settings.simulate_here = ((loaded.flags & s3ds::FlagsSimHere) != 0);

			scene.instances.particles.emplace(name, instance);
		}
	}

	{ //light
		std::vector< s3ds::Light_Instance > delta_light_instances;
		read(from, s3ds::Light_Instances_fourcc, &delta_light_instances);

		for (auto const &loaded : delta_light_instances) {
			std::string name = get_string("Light_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Light_Instance", name);

			std::shared_ptr< Instance::Delta_Light > instance = std::make_shared< Instance::Delta_Light >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Light_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.light != static_cast<uint32_t>(-1)) {
				if (loaded.light >= index_to_delta_light.size()) throw std::runtime_error(file_info() + "Light_Instance with out-of-range delta_light.");
				instance->light = index_to_delta_light[loaded.light];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);

			scene.instances.delta_lights.emplace(name, instance);
		}
	}

	{ //environment
		std::vector< s3ds::Environment_Instance > env_light_instances;
		read(from, s3ds::Environment_Instances_fourcc, &env_light_instances);

		for (auto const &loaded : env_light_instances) {
			std::string name = get_string("Environment_Instance name", loaded.name_begin, loaded.name_end);
			check_name("Environment_Instance", name);

			std::shared_ptr< Instance::Environment_Light > instance = std::make_shared< Instance::Environment_Light >();
			if (loaded.transform != static_cast<uint32_t>(-1)) {
				if (loaded.transform >= index_to_transform.size()) throw std::runtime_error(file_info() + "Environment_Instance with out-of-range transform.");
				instance->transform = index_to_transform[loaded.transform];
			}
			if (loaded.light != static_cast<uint32_t>(-1)) {
				if (loaded.light >= index_to_env_light.size()) throw std::runtime_error(file_info() + "Environment_Instance with out-of-range env_light.");
				instance->light = index_to_env_light[loaded.light];
			}
			instance->settings.visible = ((loaded.flags & s3ds::FlagsVisible) != 0);

			scene.instances.env_lights.emplace(name, instance);
		}
	}

	uint32_t bytes_read = static_cast<uint32_t>(from.tellg() - whence);

	if (bytes_read != header.bytes + 8) {
		warn("%sHeader says %d bytes but read %d bytes.", file_info().c_str(), header.bytes, bytes_read - 8); //TODO: this is actually a flaw in the file, should probably just throw.
	}

	return scene;
}

void Scene::save(std::ostream& to) const {
	//file contents, in order:
	s3ds::Header header;
	std::vector< char > f_strings;
	std::vector< uint8_t > f_texture_data;
	std::vector< s3ds::Texture > f_textures;
	std::vector< s3ds::Material > f_materials;
	std::vector< s3ds::Transform > f_transforms;
	std::vector< s3ds::Camera > f_cameras;

	std::vector< s3ds::Halfedge > f_halfedges;
	std::vector< s3ds::Vertex > f_vertices;
	std::vector< s3ds::Edge > f_edges;
	std::vector< s3ds::Face > f_faces;
	std::vector< s3ds::Halfedge_Mesh > f_halfedge_meshes;

	std::vector< s3ds::Halfedge > f_skinned_halfedges;
	std::vector< s3ds::Weight > f_skinned_weights;
	std::vector< s3ds::Skinned_Vertex > f_skinned_vertices;
	std::vector< s3ds::Edge > f_skinned_edges;
	std::vector< s3ds::Face > f_skinned_faces;
	std::vector< s3ds::Bone > f_skinned_bones;
	std::vector< s3ds::Handle > f_skinned_handles;
	std::vector< s3ds::Skinned_Mesh > f_skinned_meshes;

	std::vector< s3ds::Shape > f_shapes;

	std::vector< s3ds::Particle > f_particles;
	std::vector< s3ds::Particle_System > f_particle_systems;

	std::vector< s3ds::Light > f_lights;
	std::vector< s3ds::Environment > f_environments;

	std::vector< s3ds::Camera_Instance > f_camera_instances;
	std::vector< s3ds::Mesh_Instance > f_mesh_instances;
	std::vector< s3ds::Skinned_Mesh_Instance > f_skinned_mesh_instances;
	std::vector< s3ds::Shape_Instance > f_shape_instances;
	std::vector< s3ds::Particles_Instance > f_particles_instances;
	std::vector< s3ds::Light_Instance > f_light_instances;
	std::vector< s3ds::Environment_Instance > f_environment_instances;

	//---- fill in the data: ----
	memcpy(header.fourcc, s3ds::Header_fourcc, 4);
	//header.bytes: filled in later
	header.version = 0;

	std::unordered_map<Texture const*, uint32_t> texture_to_index;
	// save textures
	{
		for (auto const& [name, texture] : this->textures) {
			s3ds::Texture load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			if (Textures::Constant const *cptr = std::get_if< Textures::Constant >(&texture->texture)) {
				auto &val = *cptr;
				load.type = s3ds::Texture::Constant;
				s3ds::TextureConstantData tcd;

				tcd.r = val.color.r;
				tcd.g = val.color.g;
				tcd.b = val.color.b;
				tcd.scale = val.scale;

				load.data_begin = static_cast<uint32_t>(f_texture_data.size());
				f_texture_data.insert(f_texture_data.end(), reinterpret_cast<const char*>(&tcd), reinterpret_cast<const char*>(&tcd) + sizeof(tcd));
				load.data_end = static_cast<uint32_t>(f_texture_data.size());
			} else if (Textures::Image const *iptr = std::get_if< Textures::Image >(&texture->texture)) {
				auto &val = *iptr;
				load.type = s3ds::Texture::Image;
				s3ds::TextureImageData tid;

				if (val.sampler == Textures::Image::Sampler::nearest) {
					tid.interpolation = s3ds::TextureImageData::Nearest;
				} else if (val.sampler == Textures::Image::Sampler::bilinear) {
					tid.interpolation = s3ds::TextureImageData::Bilinear;
				} else if (val.sampler == Textures::Image::Sampler::trilinear) {
					tid.interpolation = s3ds::TextureImageData::Trilinear;
				} else {
					throw std::runtime_error("Texture with image has unknown interpolation type '" + std::to_string(uint32_t(val.sampler)) + "'." );
				}

				load.data_begin = static_cast<uint32_t>(f_texture_data.size());
				f_texture_data.insert(f_texture_data.end(), reinterpret_cast<const char*>(&tid), reinterpret_cast<const char*>(&tid) + sizeof(tid));
				std::vector<uint8_t> encoded = val.image.encode();
				f_texture_data.insert(f_texture_data.end(), encoded.begin(), encoded.end());
				load.data_end = static_cast<uint32_t>(f_texture_data.size());
			} else {
				throw std::runtime_error("Texture of unknown type.");
			}

			f_textures.emplace_back(load);
			texture_to_index[texture.get()] = static_cast<uint32_t>(texture_to_index.size());
		}
	}

	std::unordered_map<Material const*, uint32_t> material_to_index;
	// save materials
	{
		for (auto const& [name, material] : this->materials) {
			s3ds::Material load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			std::visit(overloaded {
						   [&](Materials::Lambertian& val) {
							   load.type = s3ds::Material::Lambertian;
							   load.albedo = texture_to_index.at(val.albedo.lock().get());
						   },
						   [&](Materials::Mirror& val) {
							   load.type = s3ds::Material::Mirror;
							   load.reflectance = texture_to_index.at(val.reflectance.lock().get());
						   },
						   [&](Materials::Refract& val) {
							   load.type = s3ds::Material::Refract;
							   load.transmittance = texture_to_index.at(val.transmittance.lock().get());
							   load.ior = val.ior;
						   },
						   [&](Materials::Glass& val) {
							   load.type = s3ds::Material::Glass;
							   load.reflectance = texture_to_index.at(val.reflectance.lock().get());
							   load.transmittance = texture_to_index.at(val.transmittance.lock().get());
							   load.ior = val.ior;
						   },
						   [&](Materials::Emissive& val) {
							   load.type = s3ds::Material::Emissive;
							   load.emission = texture_to_index.at(val.emissive.lock().get());
						   },
			}, material->material);

			f_materials.emplace_back(load);
			material_to_index[material.get()] = static_cast<uint32_t>(material_to_index.size());
		}
	}

	std::unordered_map<Transform const*, uint32_t> transform_to_index;
	// save transforms, in topological order
	{
		transform_to_index.reserve(this->transforms.size());

		std::unordered_map< Transform const *, std::string > transform_to_name;
		for (auto const& [name, transform] : this->transforms) {
			transform_to_name.emplace(transform.get(), name);
		}

		std::function< uint32_t(Transform const *) > save_transform = [&](Transform const *transform) {
			{ //already saved?
				auto f = transform_to_index.find(transform);
				if (f != transform_to_index.end()) return f->second;
			}

			std::string name = transform_to_name.at(transform);

			s3ds::Transform load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			if (auto parent = transform->parent.lock()) {
				load.parent = save_transform(parent.get());
			} else {
				load.parent = static_cast<uint32_t>(-1);
			}

			load.translation[0] = transform->translation.x;
			load.translation[1] = transform->translation.y;
			load.translation[2] = transform->translation.z;
			load.rotation[0] = transform->rotation.y;
			load.rotation[1] = transform->rotation.z;
			load.rotation[2] = transform->rotation.w;
			load.rotation[3] = transform->rotation.x;
			load.scale[0] = transform->scale.x;
			load.scale[1] = transform->scale.y;
			load.scale[2] = transform->scale.z;

			f_transforms.emplace_back(load);

			auto ret = transform_to_index.emplace(transform, static_cast<uint32_t>(transform_to_index.size()));
			assert(ret.second);

			return ret.first->second;
		};

		for (auto const& [name, transform] : this->transforms) {
			save_transform(transform.get());
		}
	}

	std::unordered_map<Camera const*, uint32_t> camera_to_index;
	// save cameras
	{
		camera_to_index.reserve(this->cameras.size());
		for (auto const& [name, camera] : this->cameras) {
			s3ds::Camera load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.fov = camera->vertical_fov;
			load.aspect = camera->aspect_ratio;
			load.near_ = camera->near_plane;
			load.film_width = camera->film.width;
			load.film_height = camera->film.height;
			load.film_samples = camera->film.samples;
			load.film_max_ray_depth = camera->film.max_ray_depth;
			load.film_sample_pattern = camera->film.sample_pattern;

			f_cameras.emplace_back(load);
			camera_to_index[camera.get()] = static_cast<uint32_t>(camera_to_index.size());
		}
	}

	std::unordered_map<Halfedge_Mesh const*, uint32_t> mesh_to_index;
	// save halfedge meshes
	{
		for (auto const& [name, mesh] : this->meshes) {
			s3ds::Halfedge_Mesh load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			// -- halfedges --
			std::unordered_map<Halfedge_Mesh::HalfedgeRef, uint32_t> halfedgeref_to_index;
			for (auto halfedge = mesh->halfedges.begin(); halfedge != mesh->halfedges.end(); ++halfedge) {
				//index of the next free storage slot:
				uint32_t i = static_cast<uint32_t>(f_halfedges.size()) + static_cast<uint32_t>(halfedgeref_to_index.size());
				assert(i%2 == 0); //always added in pairs

				halfedgeref_to_index.emplace(halfedge, i);
				halfedgeref_to_index.emplace(halfedge->twin, i^1);
			}

			load.halfedges_begin = static_cast<uint32_t>(f_halfedges.size());
			f_halfedges.resize(f_halfedges.size() + halfedgeref_to_index.size());
			for (auto halfedge = mesh->halfedges.begin(); halfedge != mesh->halfedges.end(); ++halfedge) {
				s3ds::Halfedge load_halfedge;
				load_halfedge.corner_uv[0] = halfedge->corner_uv.x;
				load_halfedge.corner_uv[1] = halfedge->corner_uv.y;
				load_halfedge.corner_normal[0] = halfedge->corner_normal.x;
				load_halfedge.corner_normal[1] = halfedge->corner_normal.y;
				load_halfedge.corner_normal[2] = halfedge->corner_normal.z;
				load_halfedge.next = halfedgeref_to_index[halfedge->next];

				uint32_t j = halfedgeref_to_index.at(halfedge);
				f_halfedges.at(j) = load_halfedge;
			}
			load.halfedges_end = static_cast<uint32_t>(f_halfedges.size());

			// -- vertices --
			load.vertices_begin = static_cast<uint32_t>(f_vertices.size());
			for (auto vertex = mesh->vertices.begin(); vertex != mesh->vertices.end(); ++vertex) {
				s3ds::Vertex load_vertex;
				load_vertex.position[0] = vertex->position.x;
				load_vertex.position[1] = vertex->position.y;
				load_vertex.position[2] = vertex->position.z;
				load_vertex.halfedge = halfedgeref_to_index[vertex->halfedge];

				f_vertices.emplace_back(load_vertex);
			}
			load.vertices_end = static_cast<uint32_t>(f_vertices.size());

			// -- edges --
			load.edges_begin = static_cast<uint32_t>(f_edges.size());
			for (auto edge = mesh->edges.begin(); edge != mesh->edges.end(); ++edge) {
				s3ds::Edge load_edge;
				load_edge.sharp_flag = edge->sharp ? s3ds::Edge::Sharp : s3ds::Edge::Smooth;
				load_edge.halfedge = halfedgeref_to_index[edge->halfedge];

				f_edges.emplace_back(load_edge);
			}
			load.edges_end = static_cast<uint32_t>(f_edges.size());

			// -- faces --
			load.faces_begin = static_cast<uint32_t>(f_faces.size());
			for (auto face = mesh->faces.begin(); face != mesh->faces.end(); ++face) {
				s3ds::Face load_face;
				load_face.boundary_flag = face->boundary ? s3ds::Face::Boundary : s3ds::Face::Surface;
				load_face.halfedge = halfedgeref_to_index[face->halfedge];

				f_faces.emplace_back(load_face);
			}
			load.faces_end = static_cast<uint32_t>(f_faces.size());

			f_halfedge_meshes.emplace_back(load);
			mesh_to_index[mesh.get()] = static_cast<uint32_t>(mesh_to_index.size());
		}
	}

	std::unordered_map<Skinned_Mesh const*, uint32_t> skinned_mesh_to_index;
	// save skinned meshes
	{
		for (auto const& [name, skinned_mesh] : this->skinned_meshes) {
			s3ds::Skinned_Mesh load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			// -- bones --
			// (doing a bit out-of-order so bones_begin is available for vertex weight saving)
			load.bones_begin = static_cast<uint32_t>(f_skinned_bones.size());
			for (auto const& bone : skinned_mesh->skeleton.bones) {
				s3ds::Bone load_bone;
				load_bone.parent = (bone.parent == -1U ? -1U : load.bones_begin + bone.parent);
				load_bone.extent[0] = bone.extent.x;
				load_bone.extent[1] = bone.extent.y;
				load_bone.extent[2] = bone.extent.z;
				load_bone.pose[0] = bone.pose.x;
				load_bone.pose[1] = bone.pose.y;
				load_bone.pose[2] = bone.pose.z;
				load_bone.radius = bone.radius;
				if (bone.channel_id != f_skinned_bones.size() - load.bones_begin) info("Bone channel_id of %u will be %u on load, messing up animations.", bone.channel_id, uint32_t(f_skinned_bones.size() - load.bones_begin));
				f_skinned_bones.emplace_back(load_bone);
			}
			load.bones_end = static_cast<uint32_t>(f_skinned_bones.size());

			// -- halfedges --
			std::unordered_map<Halfedge_Mesh::HalfedgeRef, uint32_t> halfedgeref_to_index;
			for (auto halfedge = skinned_mesh->mesh.halfedges.begin(); halfedge != skinned_mesh->mesh.halfedges.end(); ++halfedge) {
				uint32_t i = static_cast<uint32_t>(f_skinned_halfedges.size()) + static_cast<uint32_t>(halfedgeref_to_index.size());
				halfedgeref_to_index.emplace(halfedge, i);
				halfedgeref_to_index.emplace(halfedge->twin, i^1);
			}

			load.halfedges_begin = static_cast<uint32_t>(f_skinned_halfedges.size());
			f_skinned_halfedges.resize(f_skinned_halfedges.size() + halfedgeref_to_index.size());
			for (auto halfedge = skinned_mesh->mesh.halfedges.begin(); halfedge != skinned_mesh->mesh.halfedges.end(); ++halfedge) {
				s3ds::Halfedge load_halfedge;
				load_halfedge.corner_uv[0] = halfedge->corner_uv.x;
				load_halfedge.corner_uv[1] = halfedge->corner_uv.y;
				load_halfedge.corner_normal[0] = halfedge->corner_normal.x;
				load_halfedge.corner_normal[1] = halfedge->corner_normal.y;
				load_halfedge.corner_normal[2] = halfedge->corner_normal.z;
				load_halfedge.next = halfedgeref_to_index[halfedge->next];

				uint32_t j = halfedgeref_to_index[halfedge];
				f_skinned_halfedges[j] = load_halfedge;
			}
			load.halfedges_end = static_cast<uint32_t>(f_skinned_halfedges.size());

			// -- vertices --
			load.vertices_begin = static_cast<uint32_t>(f_skinned_vertices.size());
			for (auto vertex = skinned_mesh->mesh.vertices.begin(); vertex != skinned_mesh->mesh.vertices.end(); ++vertex) {
				s3ds::Skinned_Vertex load_vertex;
				load_vertex.position[0] = vertex->position.x;
				load_vertex.position[1] = vertex->position.y;
				load_vertex.position[2] = vertex->position.z;
				load_vertex.halfedge = halfedgeref_to_index[vertex->halfedge];

				load_vertex.weights_begin = static_cast<uint32_t>(f_skinned_weights.size());
				for (auto & bone_weight : vertex->bone_weights) {
					s3ds::Weight load_weight;
					load_weight.weight = bone_weight.weight;
					load_weight.bone = bone_weight.bone + load.bones_begin;

					f_skinned_weights.emplace_back(load_weight);
				}
				load_vertex.weights_end = static_cast<uint32_t>(f_skinned_weights.size());

				f_skinned_vertices.emplace_back(load_vertex);
			}
			load.vertices_end = static_cast<uint32_t>(f_skinned_vertices.size());

			// -- edges --
			load.edges_begin = static_cast<uint32_t>(f_skinned_edges.size());
			for (auto edge = skinned_mesh->mesh.edges.begin(); edge != skinned_mesh->mesh.edges.end(); ++edge) {
				s3ds::Edge load_edge;
				load_edge.sharp_flag = edge->sharp ? s3ds::Edge::Sharp : s3ds::Edge::Smooth;
				load_edge.halfedge = halfedgeref_to_index[edge->halfedge];

				f_skinned_edges.emplace_back(load_edge);
			}
			load.edges_end = static_cast<uint32_t>(f_skinned_edges.size());

			// -- faces --
			load.faces_begin = static_cast<uint32_t>(f_skinned_faces.size());
			for (auto face = skinned_mesh->mesh.faces.begin(); face != skinned_mesh->mesh.faces.end(); ++face) {
				s3ds::Face load_face;
				load_face.boundary_flag = face->boundary ? s3ds::Face::Boundary : s3ds::Face::Surface;
				load_face.halfedge = halfedgeref_to_index[face->halfedge];

				f_skinned_faces.emplace_back(load_face);
			}
			load.faces_end = static_cast<uint32_t>(f_skinned_faces.size());

			
			// -- ik handles --
 			load.handles_begin = static_cast<uint32_t>(f_skinned_handles.size());
 			for (auto const& handle : skinned_mesh->skeleton.handles) {
 				s3ds::Handle load_handle;
 				load_handle.bone = handle.bone + load.bones_begin;
 				load_handle.target[0] = handle.target.x;
 				load_handle.target[1] = handle.target.y;
 				load_handle.target[2] = handle.target.z;
 				load_handle.enabled_flag = (handle.enabled ? 1 : 0);
				if (handle.channel_id != f_skinned_handles.size() - load.handles_begin) info("Handle channel_id of %u will be %u on load, messing up animations.", handle.channel_id, uint32_t(f_skinned_handles.size() - load.handles_begin));
 				f_skinned_handles.emplace_back(load_handle);
 			}
 			load.handles_end = static_cast<uint32_t>(f_skinned_handles.size());

			load.base[0] = skinned_mesh->skeleton.base.x;
			load.base[1] = skinned_mesh->skeleton.base.y;
			load.base[2] = skinned_mesh->skeleton.base.z;

			f_skinned_meshes.emplace_back(load);
			skinned_mesh_to_index[skinned_mesh.get()] = static_cast<uint32_t>(skinned_mesh_to_index.size());
		}
	}

	std::unordered_map<Shape const*, uint32_t> shape_to_index;
	// save shapes
	{
		for (auto const& [name, shape] : this->shapes) {
			s3ds::Shape load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			std::visit(
				[&](Shapes::Sphere const& val) {
					load.type = s3ds::Shape::Sphere;
					load.radius = val.radius;
				},
				shape->shape);

			f_shapes.emplace_back(load);
			shape_to_index[shape.get()] = static_cast<uint32_t>(shape_to_index.size());
		}
	}

	std::unordered_map<Particles const*, uint32_t> particles_to_index;
	// save particle systems
	{
		if (!this->particles.empty()) warn("s3d save for particles is out of date! It doesn't save seeds or 3d gravity.");
		for (auto const& [name, particle_system] : this->particles) {
			s3ds::Particle_System load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.gravity = -particle_system->gravity.y;
			load.scale = particle_system->radius;
			load.initial_velocity = particle_system->initial_velocity;
			load.spread_angle = particle_system->spread_angle;
			load.lifetime = particle_system->lifetime;
			load.pps = particle_system->rate;
			load.step_size = particle_system->step_size;
			//(seed isn't saved)

			load.particles_begin = static_cast<uint32_t>(f_particles.size());
			for (auto & i : particle_system->particles) {
				s3ds::Particle particle;
				particle.position[0] = i.position.x;
				particle.position[1] = i.position.y;
				particle.position[2] = i.position.z;
				particle.velocity[0] = i.velocity.x;
				particle.velocity[1] = i.velocity.y;
				particle.velocity[2] = i.velocity.z;
				particle.age = i.age;
				f_particles.emplace_back(particle);
			}
			load.particles_end = static_cast<uint32_t>(f_particles.size());

			f_particle_systems.emplace_back(load);
			particles_to_index[particle_system.get()] = static_cast<uint32_t>(particles_to_index.size());
		}
	}

	std::unordered_map<Delta_Light const*, uint32_t> delta_light_to_index;
	// save lights
	{
		for (auto const& [name, delta_light] : this->delta_lights) {
			s3ds::Light load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			std::visit(overloaded{
						   [&](Delta_Lights::Point const& val) {
							   load.type = s3ds::Light::Point;
							   load.color[0] = val.color.r;
							   load.color[1] = val.color.g;
							   load.color[2] = val.color.b;
							   load.intensity = val.intensity;
						   },
						   [&](Delta_Lights::Directional const& val) {
							   load.type = s3ds::Light::Directional;
							   load.color[0] = val.color.r;
							   load.color[1] = val.color.g;
							   load.color[2] = val.color.b;
							   load.intensity = val.intensity;
						   },
						   [&](Delta_Lights::Spot const& val) {
							   load.type = s3ds::Light::Spot;
							   load.color[0] = val.color.r;
							   load.color[1] = val.color.g;
							   load.color[2] = val.color.b;
							   load.intensity = val.intensity;
							   load.inner_angle = val.inner_angle;
							   load.outer_angle = val.outer_angle;
						   }},
			           delta_light->light);

			f_lights.emplace_back(load);
			delta_light_to_index[delta_light.get()] = static_cast<uint32_t>(delta_light_to_index.size());
		}
	}

	std::unordered_map<Environment_Light const*, uint32_t> env_light_to_index;
	// save environment lights
	{
		for (auto const& [name, env_light] : this->env_lights) {
			s3ds::Environment load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			std::visit(overloaded{
						   [&](Environment_Lights::Hemisphere const& val) {
							   load.type = s3ds::Environment::Hemisphere;
							   load.intensity = val.intensity;
							   load.texture = texture_to_index.at(val.radiance.lock().get());
						   },
						   [&](Environment_Lights::Sphere const& val) {
							   load.type = s3ds::Environment::Sphere;
							   load.intensity = val.intensity;
							   load.texture = texture_to_index.at(val.radiance.lock().get());
						   },
					   },
			           env_light->light);

			f_environments.emplace_back(load);
			env_light_to_index[env_light.get()] = static_cast<uint32_t>(env_light_to_index.size());
		}
	}

	// - - - - instances - - - -
	// camera
	{
		for (auto const& [name, camera_instance] : this->instances.cameras) {
			s3ds::Camera_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(camera_instance->transform.lock().get());
			load.camera = camera_to_index.at(camera_instance->camera.lock().get());

			f_camera_instances.emplace_back(load);
		}
	}

	auto settings_to_flags = [&](Instance::Geometry_Settings const &settings) -> uint8_t {
		uint8_t flags = 0;
		if (settings.visible) flags |= s3ds::FlagsVisible;

		if      (settings.draw_style == DrawStyle::Wireframe) flags |= s3ds::FlagsDrawStyleWireframe;
		else if (settings.draw_style == DrawStyle::Flat)      flags |= s3ds::FlagsDrawStyleFlat;
		else if (settings.draw_style == DrawStyle::Smooth)    flags |= s3ds::FlagsDrawStyleSmooth;
		else if (settings.draw_style == DrawStyle::Correct)   flags |= s3ds::FlagsDrawStyleCorrect;
		else warn("unknown DrawStyle %d", int(settings.draw_style));

		if      (settings.blend_style == BlendStyle::Replace) flags |= s3ds::FlagsBlendStyleReplace;
		else if (settings.blend_style == BlendStyle::Add)     flags |= s3ds::FlagsBlendStyleAdd;
		else if (settings.blend_style == BlendStyle::Over)    flags |= s3ds::FlagsBlendStyleOver;
		else warn("unknown BlendStyle %d", int(settings.blend_style));

		if      (settings.depth_style == DepthStyle::Always)  flags |= s3ds::FlagsDepthStyleAlways;
		else if (settings.depth_style == DepthStyle::Never)   flags |= s3ds::FlagsDepthStyleNever;
		else if (settings.depth_style == DepthStyle::Less)    flags |= s3ds::FlagsDepthStyleLess;
		else warn("unknown DepthStyle %d", int(settings.depth_style));

		return flags;
	};

	// mesh
	{
		for (auto const& [name, mesh_instance] : this->instances.meshes) {
			s3ds::Mesh_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(mesh_instance->transform.lock().get());
			load.item = mesh_to_index.at(mesh_instance->mesh.lock().get());
			load.material = material_to_index.at(mesh_instance->material.lock().get());
			load.flags = settings_to_flags(mesh_instance->settings);

			f_mesh_instances.emplace_back(load);
		}
	}

	// skinned mesh
	{
		for (auto const& [name, skinned_mesh_instance] : this->instances.skinned_meshes) {
			s3ds::Skinned_Mesh_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(skinned_mesh_instance->transform.lock().get());
			load.item = skinned_mesh_to_index.at(skinned_mesh_instance->mesh.lock().get());
			load.material = material_to_index.at(skinned_mesh_instance->material.lock().get());
			load.flags = settings_to_flags(skinned_mesh_instance->settings);

			f_skinned_mesh_instances.emplace_back(load);
		}
	}

	// shape
	{
		for (auto const& [name, shape_instance] : this->instances.shapes) {
			s3ds::Shape_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(shape_instance->transform.lock().get());
			load.item = shape_to_index.at(shape_instance->shape.lock().get());
			load.material = material_to_index.at(shape_instance->material.lock().get());
			load.flags = settings_to_flags(shape_instance->settings);

			f_shape_instances.emplace_back(load);
		}
	}

	// particles
	{
		for (auto const& [name, particle_instance] : this->instances.particles) {
			s3ds::Particles_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(particle_instance->transform.lock().get());
			load.mesh = mesh_to_index.at(particle_instance->mesh.lock().get());
			load.material = material_to_index.at(particle_instance->material.lock().get());
			load.particles = particles_to_index.at(particle_instance->particles.lock().get());
			load.flags = static_cast<uint8_t>(particle_instance->settings.simulate_here) << 2 |
			             static_cast<uint8_t>(particle_instance->settings.wireframe) << 1 |
						 static_cast<uint8_t>(particle_instance->settings.visible);

			f_particles_instances.emplace_back(load);
		}
	}

	// light
	{
		for (auto const& [name, light_instance] : this->instances.delta_lights) {
			s3ds::Light_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(light_instance->transform.lock().get());
			load.light = delta_light_to_index.at(light_instance->light.lock().get());
			load.flags = static_cast<uint8_t>(light_instance->settings.visible);

			f_light_instances.emplace_back(load);
		}
	}

	// environment
	{
		for (auto const& [name, env_instance] : this->instances.env_lights) {
			s3ds::Environment_Instance load;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(name.begin(), name.end(), std::back_inserter(f_strings));
			load.name_end = static_cast<uint32_t>(f_strings.size());

			load.transform = transform_to_index.at(env_instance->transform.lock().get());
			load.light = env_light_to_index.at(env_instance->light.lock().get());
			load.flags = static_cast<uint8_t>(env_instance->settings.visible);

			f_environment_instances.emplace_back(load);
		}
	}

	//---- write the data: ----
	header.bytes = static_cast<uint32_t>(4
		+ 8 + f_strings.size() * sizeof(f_strings[0])
		+ 8 + f_texture_data.size()  * sizeof(f_texture_data[0])
		+ 8 + f_textures.size() * sizeof(f_textures[0])
		+ 8 + f_materials.size() * sizeof(f_materials[0])
		+ 8 + f_transforms.size() * sizeof(f_transforms[0])
		+ 8 + f_cameras.size() * sizeof(f_cameras[0])
		+ 8 + f_halfedges.size() * sizeof(f_halfedges[0])
		+ 8 + f_vertices.size() * sizeof(f_vertices[0])
		+ 8 + f_edges.size() * sizeof(f_edges[0])
		+ 8 + f_faces.size() * sizeof(f_faces[0])
		+ 8 + f_halfedge_meshes.size() * sizeof(f_halfedge_meshes[0])
		+ 8 + f_skinned_halfedges.size() * sizeof(f_skinned_halfedges[0])
		+ 8 + f_skinned_weights.size() * sizeof(f_skinned_weights[0])
		+ 8 + f_skinned_vertices.size() * sizeof(f_skinned_vertices[0])
		+ 8 + f_skinned_edges.size() * sizeof(f_skinned_edges[0])
		+ 8 + f_skinned_faces.size() * sizeof(f_skinned_faces[0])
		+ 8 + f_skinned_bones.size() * sizeof(f_skinned_bones[0])
		+ 8 + f_skinned_handles.size() * sizeof(f_skinned_handles[0])
		+ 8 + f_skinned_meshes.size() * sizeof(f_skinned_meshes[0])
		+ 8 + f_shapes.size() * sizeof(f_shapes[0])
		+ 8 + f_particles.size() * sizeof(f_particles[0])
		+ 8 + f_particle_systems.size() * sizeof(f_particle_systems[0])
		+ 8 + f_lights.size() * sizeof(f_lights[0])
		+ 8 + f_environments.size() * sizeof(f_environments[0])
		+ 8 + f_camera_instances.size() * sizeof(f_camera_instances[0])
		+ 8 + f_mesh_instances.size() * sizeof(f_mesh_instances[0])
		+ 8 + f_skinned_mesh_instances.size() * sizeof(f_skinned_mesh_instances[0])
		+ 8 + f_shape_instances.size() * sizeof(f_shape_instances[0])
		+ 8 + f_particles_instances.size() * sizeof(f_particles_instances[0])
		+ 8 + f_light_instances.size() * sizeof(f_light_instances[0])
		+ 8 + f_environment_instances.size() * sizeof(f_environment_instances[0])
	);

	auto whence = to.tellp();

	to.write(reinterpret_cast< const char * >(&header), sizeof(header));
	write(to, s3ds::Strings_fourcc, f_strings);
	write(to, s3ds::Texture_Data_fourcc, f_texture_data);
	write(to, s3ds::Textures_fourcc, f_textures);
	write(to, s3ds::Materials_fourcc, f_materials);
	write(to, s3ds::Transforms_fourcc, f_transforms);
	write(to, s3ds::Cameras_fourcc, f_cameras);
	write(to, s3ds::Halfedges_fourcc, f_halfedges);
	write(to, s3ds::Vertices_fourcc, f_vertices);
	write(to, s3ds::Edges_fourcc, f_edges);
	write(to, s3ds::Faces_fourcc, f_faces);
	write(to, s3ds::Halfedge_Meshes_fourcc, f_halfedge_meshes);
	write(to, s3ds::Halfedges_fourcc, f_skinned_halfedges);
	write(to, s3ds::Weights_fourcc, f_skinned_weights);
	write(to, s3ds::Skinned_Vertices_fourcc, f_skinned_vertices);
	write(to, s3ds::Edges_fourcc, f_skinned_edges);
	write(to, s3ds::Faces_fourcc, f_skinned_faces);
	write(to, s3ds::Bones_fourcc, f_skinned_bones);
	write(to, s3ds::Handles_fourcc, f_skinned_handles);
	write(to, s3ds::Skinned_Meshes_fourcc, f_skinned_meshes);
	write(to, s3ds::Shapes_fourcc, f_shapes);
	write(to, s3ds::Particles_fourcc, f_particles);
	write(to, s3ds::Particle_Systems_fourcc, f_particle_systems);
	write(to, s3ds::Lights_fourcc, f_lights);
	write(to, s3ds::Environments_fourcc, f_environments);
	write(to, s3ds::Camera_Instances_fourcc, f_camera_instances);
	write(to, s3ds::Mesh_Instances_fourcc, f_mesh_instances);
	write(to, s3ds::Skinned_Mesh_Instances_fourcc, f_skinned_mesh_instances);
	write(to, s3ds::Shape_Instances_fourcc, f_shape_instances);
	write(to, s3ds::Particles_Instances_fourcc, f_particles_instances);
	write(to, s3ds::Light_Instances_fourcc, f_light_instances);
	write(to, s3ds::Environment_Instances_fourcc, f_environment_instances);

	auto wrote = static_cast<uint32_t>(to.tellp() - whence);

	if (wrote != header.bytes + 8) {
		warn("Marked scene header with %d bytes but actually wrote %d bytes past the header.", header.bytes, wrote - 8);
	}

}

Animator Animator::load(std::istream& from) {

	//keep track of the # of bytes read:
	auto whence = from.tellg();

	auto file_info = [&]() -> std::string {
		return "[at " + std::to_string(from.tellg()) + "] ";
	};

	Animator animator;

	//starts with animator header
	s3da::Header header;
	if (!from.read(reinterpret_cast< char * >(&header), sizeof(header))) throw std::runtime_error(file_info() + "Failed to read s3da header.");

	if (std::memcmp(s3da::Header_fourcc, header.fourcc, 4) != 0) throw std::runtime_error(file_info() + "Got fourcc '" + std::string(header.fourcc, 4) + "', expected '" + std::string(s3da::Header_fourcc, 4) + "'.");
	
	if (header.version > 0) throw std::runtime_error(file_info() + "Version " + std::to_string(header.version) + " is newer than latest supported (0).");

	//keep track of the names used:
	std::unordered_set< std::pair< std::string, std::string > > paths;
	auto check_path = [&](const char *what, std::string const &resource, std::string const &channel) {
		if (!paths.emplace(std::make_pair(resource, channel)).second) throw std::runtime_error(file_info() + std::string(what) + " has duplicated resource '" + resource + "' and channel '" + channel + "'.");
	};

	//helpful for checking ranges:
	#define CHECK_RANGE(Thing, items, begin, end) \
		if ((begin) > (end) || (end) > (items).size()) throw std::runtime_error(file_info() + std::string(Thing) + " has invalid " #items " range[" + std::to_string(begin) + ", " + std::to_string(end) + ") of " + std::to_string((items).size()) + ".")

	//strings chunk:
	std::vector< char > strings;
	read(from, s3da::Strings_fourcc, &strings);

	auto get_string = [&](std::string const &what, uint32_t begin, uint32_t end) -> std::string {
		if (begin > end || end > strings.size()) throw std::runtime_error(file_info() + "String " + what + " has invalid range [" + std::to_string(begin) + "," + std::to_string(end) + ") of " + std::to_string(strings.size()) + " strings bytes.");
		return std::string(strings.begin() + begin, strings.begin() + end);
	};

	std::unordered_map<Path, Channel_Spline> splines;
	{ //load splines:
		//spline data chunk (bytes):
		std::vector< uint8_t > spline_data;
		read(from, s3da::Spline_Data_fourcc, &spline_data);
		//actual spline structures:
		std::vector< s3da::Spline > f_splines;
		read(from, s3da::Splines_fourcc, &f_splines);

		for (auto const &loaded : f_splines) {
			std::string resource = get_string("Resource name", loaded.name_begin, loaded.path_begin);
			std::string channel = get_string("Channel path", loaded.path_begin, loaded.path_end);
			check_path("Spline", resource, channel);
			CHECK_RANGE("Spline", spline_data, loaded.data_begin, loaded.data_end);

			Channel_Spline spline;
			if (loaded.type == s3da::Spline::Bool) {
				uint32_t type_size = sizeof(s3da::SplineBoolData);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in bool spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of bool control point size [" + std::to_string(type_size) + "].");
				
				Spline<bool> bool_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineFloatData sbd;
					memcpy(&sbd, &spline_data[i], type_size);
					bool_spline.set(sbd.time, sbd.value);
				}
				spline = bool_spline;
			} else if (loaded.type == s3da::Spline::Float) {
				uint32_t type_size = sizeof(s3da::SplineFloatData);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in float spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of float control point size [" + std::to_string(type_size) + "].");
				
				Spline<float> float_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineFloatData sfd;
					memcpy(&sfd, &spline_data[i], type_size);
					float_spline.set(sfd.time, sfd.value);
				}
				spline = float_spline;
			} else if (loaded.type == s3da::Spline::Vec2) {
				uint32_t type_size = sizeof(s3da::SplineVec2Data);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Vec2 spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Vec2 control point size [" + std::to_string(type_size) + "].");
				
				Spline<Vec2> vec2_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineVec2Data s2d;
					memcpy(&s2d, &spline_data[i], type_size);
					vec2_spline.set(s2d.time, Vec2(s2d.value[0], s2d.value[1]));
				}
				spline = vec2_spline;
			} else if (loaded.type == s3da::Spline::Vec3) {
				uint32_t type_size = sizeof(s3da::SplineVec3Data);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Vec3 spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Vec3 control point size [" + std::to_string(type_size) + "].");
				
				Spline<Vec3> vec3_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineVec3Data s3d;
					memcpy(&s3d, &spline_data[i], type_size);
					vec3_spline.set(s3d.time, Vec3(s3d.value[0], s3d.value[1], s3d.value[2]));
				}
				spline = vec3_spline;
			} else if (loaded.type == s3da::Spline::Vec4) {
				uint32_t type_size = sizeof(s3da::SplineVec4Data);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Vec4 spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Vec4 control point size [" + std::to_string(type_size) + "].");
				
				Spline<Vec4> vec4_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineVec4Data s4d;
					memcpy(&s4d, &spline_data[i], type_size);
					vec4_spline.set(s4d.time, Vec4(s4d.value[0], s4d.value[1], s4d.value[2], s4d.value[3]));
				}
				spline = vec4_spline;
			} else if (loaded.type == s3da::Spline::Quat) {
				uint32_t type_size = sizeof(s3da::SplineQuatData);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Quat spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Quat control point size [" + std::to_string(type_size) + "].");
				
				Spline<Quat> quat_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineQuatData sqd;
					memcpy(&sqd, &spline_data[i], type_size);
					quat_spline.set(sqd.time, Quat(sqd.value[0], sqd.value[1], sqd.value[2], sqd.value[3]));
				}
				spline = quat_spline;
			} else if (loaded.type == s3da::Spline::Spectrum) {
				uint32_t type_size = sizeof(s3da::SplineSpectrumData);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Spectrum spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Spectrum control point size [" + std::to_string(type_size) + "].");
				
				Spline<Spectrum> spectrum_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineSpectrumData ssd;
					memcpy(&ssd, &spline_data[i], type_size);
					spectrum_spline.set(ssd.time, Spectrum(ssd.value[0], ssd.value[1], ssd.value[2]));
				}
				spline = spectrum_spline;
			} else if (loaded.type == s3da::Spline::Mat4) {
				uint32_t type_size = sizeof(s3da::SplineMat4Data);
				if ((loaded.data_end - loaded.data_begin) % type_size != 0) throw std::runtime_error(file_info() + "Bytes in Mat4 spline data [" + std::to_string(loaded.data_end-loaded.data_begin) + "] is not a multiple of Mat4 control point size [" + std::to_string(type_size) + "].");
				
				Spline<Mat4> mat4_spline;
				for (uint32_t i = loaded.data_begin; i < loaded.data_end; i += type_size) {
					s3da::SplineMat4Data smd;
					memcpy(&smd, &spline_data[i], type_size);
					mat4_spline.set(smd.time, Mat4(reinterpret_cast<std::array<float, 16>&>(smd.value)));
				}
				spline = mat4_spline;
			} 

			Path path (resource, channel);
			animator.splines.emplace(path, spline);

		}
	}

	uint32_t bytes_read = static_cast<uint32_t>(from.tellg() - whence);

	if (bytes_read != header.bytes + 8) {
		throw std::runtime_error(file_info() + "Header says " + std::to_string(header.bytes) + " bytes but read " + std::to_string(bytes_read - 8) + " bytes.");
	}

	return animator;
}

void Animator::save(std::ostream& to) const {
	// file contents, in order:
	s3da::Header header;
	std::vector< char > f_strings;
	std::vector< uint8_t > f_spline_data;
	std::vector< s3da::Spline > f_splines;

	// ---- fill in the data: ----
	memcpy(header.fourcc, s3da::Header_fourcc, 4);
	// header.bytes: filled in later
	header.version = 0;

	// save splines
	{
		for (auto const& [path, channel_spline] : this->splines) {
			s3da::Spline load;
			auto [resource_name, channel_path] = path;
			load.name_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(resource_name.begin(), resource_name.end(), std::back_inserter(f_strings));
			load.path_begin = static_cast<uint32_t>(f_strings.size());
			std::copy(channel_path.begin(), channel_path.end(), std::back_inserter(f_strings));
			load.path_end = static_cast<uint32_t>(f_strings.size());

			std::visit(overloaded{
						   [&](Spline<bool> const& val) {
							   load.type = s3da::Spline::Bool;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineBoolData sbd;
								   sbd.time = time;
								   sbd.value = val.at(time);
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&sbd), reinterpret_cast<uint8_t*>(&sbd) + sizeof(sbd));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<float> const& val) {
							   load.type = s3da::Spline::Float;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineFloatData sfd;
								   sfd.time = time;
								   sfd.value = val.at(time);
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&sfd), reinterpret_cast<uint8_t*>(&sfd) + sizeof(sfd));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Vec2> const& val) {
							   load.type = s3da::Spline::Vec2;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineVec2Data s2d;
								   s2d.time = time;
								   s2d.value[0] = val.at(time).x;
								   s2d.value[1] = val.at(time).y;
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&s2d), reinterpret_cast<uint8_t*>(&s2d) + sizeof(s2d));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Vec3> const& val) {
							   load.type = s3da::Spline::Vec3;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineVec3Data s3d;
								   s3d.time = time;
								   s3d.value[0] = val.at(time).x;
								   s3d.value[1] = val.at(time).y;
								   s3d.value[2] = val.at(time).z;
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&s3d), reinterpret_cast<uint8_t*>(&s3d) + sizeof(s3d));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Vec4> const& val) {
							   load.type = s3da::Spline::Vec4;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineVec4Data s4d;
								   s4d.time = time;
								   s4d.value[0] = val.at(time).x;
								   s4d.value[1] = val.at(time).y;
								   s4d.value[2] = val.at(time).z;
								   s4d.value[3] = val.at(time).w;
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&s4d), reinterpret_cast<uint8_t*>(&s4d) + sizeof(s4d));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Quat> const& val) {
							   load.type = s3da::Spline::Quat;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineQuatData sqd;
								   sqd.time = time;
								   sqd.value[0] = val.at(time).x;
								   sqd.value[1] = val.at(time).y;
								   sqd.value[2] = val.at(time).z;
								   sqd.value[3] = val.at(time).w;
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&sqd), reinterpret_cast<uint8_t*>(&sqd) + sizeof(sqd));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Spectrum> const& val) {
							   load.type = s3da::Spline::Spectrum;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineSpectrumData ssd;
								   ssd.time = time;
								   ssd.value[0] = val.at(time).r;
								   ssd.value[1] = val.at(time).g;
								   ssd.value[2] = val.at(time).b;
								   f_spline_data.insert(f_spline_data.end(), reinterpret_cast<uint8_t*>(&ssd), reinterpret_cast<uint8_t*>(&ssd) + sizeof(ssd));
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
						   [&](Spline<Mat4> const& val) {
							   load.type = s3da::Spline::Mat4;
							   load.data_begin = static_cast<uint32_t>(f_spline_data.size());
							   std::set<float> times = val.keys();

							   for (auto const& time : times) {
								   s3da::SplineMat4Data smd;
								   smd.time = time;
								   std::copy(val.at(time).data, val.at(time).data + 16, smd.value);
							   }

							   load.data_end = static_cast<uint32_t>(f_spline_data.size());
						   },
			}, channel_spline);

			f_splines.emplace_back(load);
		}
	}

	// ---- write the data: ----
	header.bytes =  static_cast<uint32_t>(4
		+ 8 + f_strings.size() * sizeof(f_strings[0])
		+ 8 + f_spline_data.size() * sizeof(f_spline_data[0])
		+ 8 + f_splines.size() * sizeof(f_splines[0])
	);

	auto whence = to.tellp();

	to.write(reinterpret_cast< const char* >(&header), sizeof(header));
	write(to, s3da::Strings_fourcc, f_strings);
	write(to, s3da::Spline_Data_fourcc, f_spline_data);
	write(to, s3da::Splines_fourcc, f_splines);

	auto wrote = to.tellp() - whence;

	if (wrote != header.bytes + 8) {
		warn("Marked animator header with %d bytes but actually wrote %d bytes past the header.", header.bytes, wrote - 8);
	}
}
