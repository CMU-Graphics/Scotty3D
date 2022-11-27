#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <optional>

#include "../geometry/spline.h"

class Scene;

namespace std {
template<> struct hash<pair<string, string>> {
	uint64_t operator()(const pair<string, string>& key) const {
		static const hash<string> h;
		return h(key.first) + h(key.second);
	}
};
} // namespace std

namespace sejp { struct value; }

class Animator {
public:
	// Load from stream; expects stream to start with s3da data; throws on error:
	static Animator load(std::istream& from);
	// Save to stream in s3da format:
	void save(std::ostream& to) const;

	// Load from json value in js3da format; throws on error:
	static Animator load_json(sejp::value const &from);
	// Save to stream as a json value in js3da format:
	void save_json(std::ostream &to) const;

	// Merges splines from other into this animator. Already existing paths are replaced.
	void merge(Animator&& other);

	// Resource name, channel path
	using Path = std::pair<std::string, std::string>;

	using Channel = std::variant<bool, float, Vec2, Vec3, Vec4, Quat, Spectrum, Mat4>;
	using Channel_Ref = std::variant<bool&, float&, Vec2&, Vec3&, Vec4&, Quat&, Spectrum&, Mat4&>;
	using Channel_Spline = std::variant<Spline<bool>, Spline<float>, Spline<Vec2>, Spline<Vec3>,
	                                    Spline<Vec4>, Spline<Quat>, Spline<Spectrum>, Spline<Mat4>>;

	void drive(Scene& scene, float time) const;
	void rename(const std::string& old_name, const std::string& new_name);

	std::vector< std::pair< Path, Channel_Spline > > remove_unused_channels(Scene& scene); //remove channels that refer to nothing
	void insert_channels(std::vector< std::pair< Path, Channel_Spline > > const &channels); //add channels back (intended to be used when undo()-ing remove_unused_channels)

	template<typename T> void set(const Path& path, float time, T value);
	template<typename T> std::optional<T> get(const Path& path, float time) const;
	void erase(const Path& path, float time);

	bool has_channels(Scene& scene, const std::string& name) const;
	void set_all(Scene& scene, const std::string& name, float time);
	void erase_all(Scene& scene, const std::string& name, float time);

	float max_key() const;
	void crop(float max_key);
	std::set<float> keys(const std::string& name) const;
	std::unordered_map<std::string, std::set<float>> all_keys() const;

	std::unordered_map<Path, Channel_Spline> splines;

	float frame_rate = 24.0f; //splines are timed in frames; divide frames by frame rate to get world times
};
