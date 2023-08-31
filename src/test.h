#pragma once

/*
 * Testing support.
 *  To make a test case, wrap it in a global Test object.
 */

#include <functional>
#include <stdexcept>
#include <string>
#include <optional>
#include <memory>

class Scene;
class Halfedge_Mesh;
class Indexed_Mesh;
struct Spectrum;
class Bone;
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat4;
struct Ray;

namespace PT {
	struct Trace;
	class Tri_Mesh;
}

class Test {
public:
	// Make a test case:
	Test(const std::string& name,          // name of test case (Assignment.specific_test_name)
	     const std::function<void()>& test // test function; throw Test::error on failure
	);

	// Throw this from a failing test:
	class error : public std::runtime_error {
	public:
		error(const std::string& why) : std::runtime_error(why) {
		}
	};

	// Throw this from a test that can't be run in the current build configuration:
	class ignored : public std::runtime_error {
	public:
		ignored(const std::string& why) : std::runtime_error(why) {
		}
	};

	// run test cases:
	static bool run_tests(const std::string& prefix // run test cases which include this string
	);

	//----- helpers ----

	static constexpr float differs_eps = 0.001f;

	static bool differs(float a, float b);
	static bool differs(Vec2 a, Vec2 b);
	static bool differs(Vec3 a, Vec3 b);
	static bool differs(Vec4 a, Vec4 b);
	static bool differs(Mat4 a, Mat4 b);
	static bool differs(Spectrum a, Spectrum b);
	static bool differs(std::vector<Vec3> a, std::vector<Vec3> b);
	static bool differs(std::vector<Mat4> a, std::vector<Mat4> b);
	static bool differs(std::vector<int> a, std::vector<int> b);

	//this is specialized for a fair few more things:
	// (see skeleton.cpp)
	template< typename T >
	static std::optional<std::string> differs(T const &a, T const &b);

	using CheckExtra = uint8_t;
	enum : CheckExtra {
		CheckIdsBit = 0x01,
		CheckBoneWeightsBit = 0x02,
		CheckCornerNormalsBit = 0x04,
		CheckCornerUVsBit = 0x08,
		CheckSharpBit = 0x10,
		CheckAllBits = 0xff
	};
	//checks position + whatever you pass in check_extra:
	static std::optional<std::string> differs(Halfedge_Mesh const &a, Halfedge_Mesh const &b, CheckExtra check_extra = CheckIdsBit);

	static void print_matrix(Mat4 matrix);
	static void print_vec3s(const std::vector<Vec3>& vec);
	static void print_floats(const std::vector<float>& vec);
	static void print_spectrums(const std::vector<Spectrum>& vec);

	// These could really be templated, but each one is only used for one type.
	static void check_constant_time(std::function<void(Halfedge_Mesh&)> op);
	static void check_log_time(std::function<void(PT::Tri_Mesh&)> op);
	static void check_linear_time(std::function<void(Halfedge_Mesh&)> op);
	static void check_loglinear_time(std::function<void(Indexed_Mesh&)> op);

	static std::string mesh_to_string(const Halfedge_Mesh& mesh);
	static bool distant_from(const Halfedge_Mesh& from, const Halfedge_Mesh& to, float scale);
	static float closest_distance(const Halfedge_Mesh& from, const Vec3& to);

	static double total_squared_error(const std::vector<double>& a, const std::vector<double>& b);
	static double total_squared_error(Spectrum a, Spectrum b);
	static double print_empirical_threshold(const std::vector<double>& ref,
	                                        const std::function<std::vector<double>()>& histogram);
	static bool run_generators; //for test cases that need to run something against reference to generate data
};
