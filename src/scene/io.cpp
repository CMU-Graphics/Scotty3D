#include "io.h"
#include "scene.h"
#include "animator.h"
#include "../lib/log.h"

#include <sejp/sejp.hpp>

#include <filesystem>
#include <fstream>

static bool is_suffix(std::string const &str, std::string const &suffix) {
	if (suffix.size() > str.size()) return false;
	return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void load(std::string const &filepath, Scene *scene_, Animator *animator_, Format format) {
	std::ifstream file(filepath, std::ios::binary);

	//try to guess format from first byte of file:
	if (format == Format::Any) {
		if (file.peek() == 's') {
			//first byte of magic number
			format = Format::Binary;
		} else if (file.peek() == '{') {
			//first byte of JSON object -- though it could be something weirder
			format = Format::JSON;
		} else {
			info("Unable to guess format of '%s' from first byte; attempting to load as JSON.", filepath.c_str());
			format = Format::JSON;
		}
	}

	Scene scene;
	Animator animator;
	if (format == Format::JSON) {
		try {
			sejp::value root = sejp::load(filepath);
			auto object = root.as_object();
			if (!object) throw std::runtime_error("root is not an object");
			auto sc = object->find("scene");
			if (sc == object->end()) {
				std::cerr << "WARNING: file does not contain a scene." << std::endl;
				scene = Scene();
			} else {
				scene = Scene::load_json(sc->second, filepath);
			}
			auto an = object->find("animator");
			if (an == object->end()) {
				std::cerr << "WARNING: file does not contain an animator." << std::endl;
				animator = Animator();
			} else {
				animator = Animator::load_json(an->second);
			}
		} catch (std::exception &e) {
			throw std::runtime_error("Failed to load '" + filepath + "' as js3d: " + e.what());
		}
	} else if (format == Format::Binary) {
		try {
			scene = Scene::load(file);
			animator = Animator::load(file);
		} catch (std::exception &e) {
			throw std::runtime_error("Failed to load '" + filepath + "' as s3d: " + e.what());
		}
	} else {
		throw std::runtime_error("Unknown format.");
	}

	if (scene_) *scene_ = std::move(scene);
	if (animator_) *animator_ = std::move(animator);
}

void save(std::string const &filepath, Scene const &scene, Animator const &animator, Format format) {
	if (format == Format::Any) {
		if (is_suffix(filepath, ".s3d")) {
			format = Format::Binary;
		} else if (is_suffix(filepath, ".js3d")) {
			format = Format::JSON;
		} else {
			throw std::runtime_error("No format specified and file extension not recognized.");
		}
	}
	if (format == Format::Binary) {
		std::ofstream file(filepath + ".temp", std::ios::binary);
		scene.save(file);
		animator.save(file);
	} else if (format == Format::JSON) {
		std::ofstream file(filepath + ".temp", std::ios::binary);
		file << "{\"scene\":";
		scene.save_json(file, filepath);
		file << ",\"animator\":";
		animator.save_json(file);
		file << "}";
	} else {
		throw std::runtime_error("Unrecognized format specified.");
	}

	std::filesystem::rename(filepath + ".temp", filepath);
}
