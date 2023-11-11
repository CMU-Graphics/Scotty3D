#pragma once

//utilities for converting to/from json (used by load_save_json)

#include <string>
#include <vector>
#include <cstdint>

namespace sejp { struct value; }
struct Spectrum;
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat4;
struct Quat;
struct SamplePattern;
class Halfedge_Mesh;

//stores string as string (this handles proper escaping)
std::string to_json(std::string const &str);
void from_json(sejp::value const &info, std::string *val);

//stores bool as bool
std::string to_json(bool val);
void from_json(sejp::value const &info, bool *val);

//stores uint32_t as number
std::string to_json(uint32_t val);
void from_json(sejp::value const &info, uint32_t *val);

//stores float as number (many digits to [one hopes] round trip well)
std::string to_json(float val);
void from_json(sejp::value const &info, float *val);

//stores Vec2 as [x,y] array
std::string to_json(Vec2 const &val);
void from_json(sejp::value const &info, Vec2 *val);

//stores Vec3 as [x,y,z] array
std::string to_json(Vec3 const &val);
void from_json(sejp::value const &info, Vec3 *val);

//stores Vec4 as [x,y,z,w] array
std::string to_json(Vec4 const &val);
void from_json(sejp::value const &info, Vec4 *val);

//stores Mat4 as column-major, 16-element array
std::string to_json(Mat4 const &val);
void from_json(sejp::value const &info, Mat4 *val);

//stores Spectrum as [r,g,b] array
std::string to_json(Spectrum const &val);
void from_json(sejp::value const &info, Spectrum *val);

//stores Quat as [x,y,z,w] array
std::string to_json(Quat const &val);
void from_json(sejp::value const &info, Quat *val);

//stores pointers into the global list of sample patterns by name:
std::string to_json(SamplePattern const *val);
void from_json(sejp::value const &info, SamplePattern const **val);

//stores halfedge mesh by base64-encoded lists of attributes:
std::string to_json(Halfedge_Mesh const &val);
void from_json(sejp::value const &info, Halfedge_Mesh *val);

//stores a vector of plain-old-data as a base64-encoded blob:
template< typename T >
std::string to_json_base64(std::vector< T > const &data, std::string const &type);
template< typename T >
void from_json_base64(sejp::value const &info, std::vector< T > *data, std::string const &type);
//also a special overload for bool vectors which bit-packs 'em:
std::string to_json_base64(std::vector< bool > const &data, std::string const &type);
void from_json_base64(sejp::value const &info, std::vector< bool > *data, std::string const &type);

//(explicitly instantiated on a few useful types at the bottom of to_json.cpp)
