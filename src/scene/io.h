#pragma once

#include <string>

class Scene;
class Animator;

//helpers and constants for loading/saving scene + animator structures:

enum class Format {
	Binary, //binary data (legacy, generally .s3d)
	JSON, //json encoded data (more flexible, generally .js3d)
	Any //determine format from filename (save) or magic number (load)
};

//load scene + animator, throws on error:
void load(std::string const &filepath, Scene *scene, Animator *animator, Format format = Format::Any);

//save scene + animator. Filepath must be '.s3d' or '.js3d' *or* format must not be Any:
void save(std::string const &filepath, Scene const &scene, Animator const &animator, Format format = Format::Any);
