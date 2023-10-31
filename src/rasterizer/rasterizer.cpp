// clang-format off
#include "rasterizer.h"
#include "../geometry/util.h"
#include "../scene/scene.h"
#include "../util/timer.h"
#include "framebuffer.h"
#include "pipeline.h"
#include "programs.h"
#include "sample_pattern.h"

struct RasterJob {
	// used to tell the job to quit early:
	bool quit = false;

	// scene data:
	using Image = Textures::Image;
	std::vector<Image> images;
	struct Material {
		Image* image; // must be non-null!
		enum class Type {
			Lambertian, // rendered with Programs::Lambertian and Blend::Replace
			Emissive,   // rendered with Programs::Unshaded and Blend::Additive
			Transparent // rendered with Programs::Lambertian, sorted back-to-front, with and
			            // Blend::Over
		} type;
	};
	std::vector<Material> materials;

	// All 3 * 3 * 3 triangle blend + depth + interpolation combinations
	using Lambertian_Triangles_Replace_Always_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Replace_Always_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Always | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Replace_Always_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Always | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Replace_Never_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Replace_Never_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Never | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Replace_Never_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Never | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Replace_Less_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Replace_Less_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Replace_Less_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Add_Always_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Add_Always_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Always | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Add_Always_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Always | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Add_Never_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Add_Never_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Never | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Add_Never_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Never | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Add_Less_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Add_Less_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Less | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Add_Less_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Less | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Over_Always_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Over_Always_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Always | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Over_Always_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Always | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Over_Never_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Over_Never_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Never | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Over_Never_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Never | Pipeline_Interp_Correct>;
	using Lambertian_Triangles_Over_Less_Flat_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	using Lambertian_Triangles_Over_Less_Smooth_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Less | Pipeline_Interp_Smooth>;
	using Lambertian_Triangles_Over_Less_Correct_Pipeline =
		Pipeline<PrimitiveType::Triangles, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Less | Pipeline_Interp_Correct>;
	// All 3 * 3 wireframe blend + depth combinations
	using Lambertian_Lines_Replace_Always_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Replace_Never_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Replace_Less_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Replace | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Add_Always_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Add_Never_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Add_Less_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Add | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Over_Always_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Always | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Over_Never_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Never | Pipeline_Interp_Flat>;
	using Lambertian_Lines_Over_Less_Pipeline =
		Pipeline<PrimitiveType::Lines, Programs::Lambertian,
	             Pipeline_Blend_Over | Pipeline_Depth_Less | Pipeline_Interp_Flat>;
	// Lambertian Vertex used for mesh manipulations
	using Lambertian_Replace_Less_Correct_Vertex =
		Lambertian_Triangles_Replace_Less_Correct_Pipeline::Vertex;

	struct Mesh {
		Halfedge_Mesh source;
		std::vector<Lambertian_Replace_Less_Correct_Vertex> lamb_triangles;
		std::vector<Lambertian_Replace_Less_Correct_Vertex> lamb_edges;
	};
	std::vector<Mesh> meshes;
	struct Instance {
		std::string name; // for DEBUG output
		Mat4 local_to_world;
		Mesh* mesh; // pointer into 'meshes' vector, above
		Material* material; // pointer into 'materials' vector, above
		DrawStyle draw_style; // draw style (Lines / Flat Triangles / Smooth Triangles / Correct Triangles)
		BlendStyle blend_style; // blend style (Blend Replace / Blend Add / Blend Over)
		DepthStyle depth_style; // depth style (Depth Always / Depth Never / Depth Less)
	};
	std::vector<Instance> instances;

	// lighting info:
	Spectrum sun_energy;
	Vec3 sun_direction;

	Spectrum sky_energy;
	Spectrum ground_energy;
	Vec3 sky_direction;

	// camera info:
	Mat4 world_to_clip; // camera.proj() * camera.world_to_local()

	// reporting function:
	std::function<void(Rasterizer::Render_Report)> report_fn;

	// output:
	Framebuffer framebuffer; // (camera.film_width) x (camera.film_height) with sampling pattern (camera.film_sampling_pattern)

	// copy data into this raster job:
	RasterJob(Scene const& scene, ::Instance::Camera const& camera,
	          std::function<void(Rasterizer::Render_Report)>&& report_fn_)
		: report_fn(report_fn_),
		  framebuffer(camera.camera.lock()->film.width, camera.camera.lock()->film.height,
	                  *SamplePattern::from_id(camera.camera.lock()->film.sample_pattern)) {

		// copy scene data:

		// Scene Textures get converted to images:
		images.reserve(1 + scene.textures.size());
		images.emplace_back(Textures::Image::Sampler::nearest,
		                    HDR_Image(1, 1, {Spectrum{1.0f, 0.0f, 1.0f}}));
		Image* const error_image = &images.back();

		// Helper to add a texture from the scene to the local data;
		// uses texture_to_local to keep track of duplicates:
		std::unordered_map<::Texture const*, Image*> texture_to_local;
		auto add_texture = [&](::Texture const& to_add) -> Image* {
			Image*& local = texture_to_local.emplace(&to_add, nullptr).first->second;
			if (!local) {
				if (Textures::Image const* image = std::get_if<Textures::Image>(&to_add.texture)) {
					images.emplace_back(image->copy());
					local = &images.back();
				} else if (Textures::Constant const* constant =
				               std::get_if<Textures::Constant>(&to_add.texture)) {
					images.emplace_back(Textures::Image::Sampler::nearest,
					                    HDR_Image(1, 1, {constant->color * constant->scale}));
					local = &images.back();
				} else {
					warn("Encountered unknown Texture variant, replacing with error image.");
					local = error_image;
				}
			}
			return local;
		};

		// Scene Materials get converted to Material structs:
		materials.reserve(1 + scene.materials.size());
		materials.emplace_back();
		materials.back().image = error_image;
		materials.back().type = Material::Type::Emissive;
		Material* const error_material = &materials.back();

		// Helper to add a material from the scene to the local copied data;
		// uses material_to_local to avoid duplicates.
		std::unordered_map<::Material const*, Material*> material_to_local;
		auto add_material = [&](::Material const& to_add) -> Material* {
			Material*& local = material_to_local.emplace(&to_add, nullptr).first->second;
			if (!local) {
				if (Materials::Lambertian const* lambertian = std::get_if<Materials::Lambertian>(&to_add.material)) {
					materials.emplace_back();
					materials.back().image = add_texture(*lambertian->albedo.lock());
					materials.back().type = Material::Type::Lambertian;
					local = &materials.back();
				} else if (Materials::Emissive const* emissive = std::get_if<Materials::Emissive>(&to_add.material)) {
					materials.emplace_back();
					materials.back().image = add_texture(*emissive->emissive.lock());
					materials.back().type = Material::Type::Emissive;
					local = &materials.back();
				} else if (Materials::Glass const* glass = std::get_if<Materials::Glass>(&to_add.material)) {
					materials.emplace_back();
					materials.back().image = add_texture(*glass->transmittance.lock());
					materials.back().type = Material::Type::Transparent;
					local = &materials.back();
				} else if (Materials::Refract const* refract = std::get_if<Materials::Refract>(&to_add.material)) {
					materials.emplace_back();
					materials.back().image = add_texture(*refract->transmittance.lock());
					materials.back().type = Material::Type::Transparent;
					local = &materials.back();
				} else {
					warn("Encountered unknown Material variant, replacing with bright magenta.");
					local = error_material;
				}
			}
			return local;
		};

		// Scene Meshes, Skinned_Meshes, and Shapes get converted to Mesh structs:
		meshes.reserve(1 + scene.meshes.size() + scene.skinned_meshes.size());
		std::unordered_map<Halfedge_Mesh const*, Mesh*> mesh_to_local;
		auto add_mesh = [&](Halfedge_Mesh const& to_add) -> Mesh* {
			Mesh*& local = mesh_to_local.emplace(&to_add, nullptr).first->second;
			if (!local) {
				meshes.emplace_back();
				meshes.back().source = to_add.copy();
				local = &meshes.back();
			}
			return local;
		};
		std::unordered_map<Skinned_Mesh const*, Mesh*> skinned_mesh_to_local;
		auto add_skinned_mesh = [&](Skinned_Mesh const& to_add) -> Mesh* {
			Mesh*& local = skinned_mesh_to_local.emplace(&to_add, nullptr).first->second;
			if (!local) {
				meshes.emplace_back();
				meshes.back().source = Halfedge_Mesh::from_indexed_mesh(to_add.posed_mesh());
				local = &meshes.back();
			}
			return local;
		};

		Mesh* sphere_mesh = nullptr; // will get set if shapes need it
		auto add_sphere = [&]() -> Mesh* {
			if (!sphere_mesh) {
				meshes.emplace_back();
				meshes.back().source = Halfedge_Mesh::from_indexed_mesh(Util::closed_sphere_mesh(1.0f, 2));
				sphere_mesh = &meshes.back();
			}
			return sphere_mesh;
		};

		// Scene instances get converted to Instances:
		instances.reserve(scene.instances.meshes.size() + scene.instances.skinned_meshes.size() +
		                  scene.instances.shapes.size());
		for (auto const& [name, to_add] : scene.instances.meshes) {
			if (!to_add->settings.visible) continue;
			instances.emplace_back();
			instances.back().name = name;
			instances.back().local_to_world = to_add->transform.lock()->local_to_world();
			instances.back().mesh = add_mesh(*to_add->mesh.lock());
			instances.back().material = add_material(*to_add->material.lock());
			instances.back().draw_style = to_add->settings.draw_style;
			instances.back().blend_style = to_add->settings.blend_style;
			instances.back().depth_style = to_add->settings.depth_style;
		}
		for (auto const& [name, to_add] : scene.instances.skinned_meshes) {
			if (!to_add->settings.visible) continue;
			instances.emplace_back();
			instances.back().name = name;
			instances.back().local_to_world = to_add->transform.lock()->local_to_world();
			instances.back().mesh = add_skinned_mesh(*to_add->mesh.lock());
			instances.back().material = add_material(*to_add->material.lock());
			instances.back().draw_style = to_add->settings.draw_style;
			instances.back().blend_style = to_add->settings.blend_style;
			instances.back().depth_style = to_add->settings.depth_style;
		}
		for (auto const& [name, to_add] : scene.instances.shapes) {
			if (!to_add->settings.visible) continue;
			if (Shapes::Sphere const* sphere =
			        std::get_if<Shapes::Sphere>(&to_add->shape.lock()->shape)) {
				// use unit sphere mesh and account for radius by scaling:
				float const r = sphere->radius;
				instances.emplace_back();
				instances.back().name = name;
				instances.back().local_to_world =
					to_add->transform.lock()->local_to_world() * Mat4::scale(Vec3{r, r, r});
				instances.back().mesh = add_sphere();
				instances.back().material = add_material(*to_add->material.lock());
				instances.back().draw_style = to_add->settings.draw_style;
				instances.back().blend_style = to_add->settings.blend_style;
				instances.back().depth_style = to_add->settings.depth_style;
			} else {
				warn("Shape %s is an unsupported variant.", name.c_str());
			}
		}

		// TODO: particles?

		// set lighting to something default-ish:

		// "sun + sky" style:
		// sun_energy = Spectrum{ 1.0f, 1.0f, 0.95f };
		// sun_direction = Vec3{ 0.25f, 0.5f, 1.0f }.unit();
		// sky_energy = Spectrum{ 0.2f, 0.2f, 0.25f };
		// ground_energy = Spectrum{ 0.01f, 0.01f, 0.01f };
		// sky_direction = Vec3{ 0.0f, 0.0f, 1.0f };

		// "headlight" + "dome" style:
		sun_energy = Spectrum{1.0f, 1.0f, 1.0f};
		sun_direction = (camera.transform.lock()->local_to_world() * Vec4(0.0f, 0.0f, -1.0f, 0.0f))
		                    .xyz()
		                    .unit();

		sky_energy = Spectrum{0.5f, 0.5f, 0.5f};
		ground_energy = Spectrum{0.01f, 0.01f, 0.01f};
		sky_direction = Vec3{0.0f, 0.0f, 1.0f};

		// TODO: could look for a Delta_Light for the sun and a Sphere or Hemisphere for the sky

		// copy camera info:
		world_to_clip =
			camera.camera.lock()->projection() * camera.transform.lock()->world_to_local();
	}

	// actually run the raster job:
	void run() {

		// helper function that caches triangle attributes for using Programs::Lambertian to draw a
		// mesh's triangles:
		auto make_lamb_triangles = [](Mesh* mesh) {
			if (!mesh->lamb_triangles.empty()) return;
			Indexed_Mesh indexed =
				Indexed_Mesh::from_halfedge_mesh(mesh->source, Indexed_Mesh::SplitEdges);
			std::vector<Indexed_Mesh::Vert> const& vertices = indexed.vertices();
			std::vector<Indexed_Mesh::Index> const& indices = indexed.indices();

			mesh->lamb_triangles.reserve(indices.size());
			for (auto i : indices) {
				Indexed_Mesh::Vert const& iv = vertices[i];
				Lambertian_Replace_Less_Correct_Vertex v;
				v.attributes[Programs::Lambertian::VA_PositionX] = iv.pos.x;
				v.attributes[Programs::Lambertian::VA_PositionY] = iv.pos.y;
				v.attributes[Programs::Lambertian::VA_PositionZ] = iv.pos.z;
				v.attributes[Programs::Lambertian::VA_NormalX] = iv.norm.x;
				v.attributes[Programs::Lambertian::VA_NormalY] = iv.norm.y;
				v.attributes[Programs::Lambertian::VA_NormalZ] = iv.norm.z;
				v.attributes[Programs::Lambertian::VA_TexCoordU] = iv.uv.x;
				v.attributes[Programs::Lambertian::VA_TexCoordV] = iv.uv.y;
				mesh->lamb_triangles.emplace_back(v);
			}
		};

		// helper function that caches triangle attributes for using Programs::Lambertian to draw
		// lines from a given mesh:
		auto make_lamb_edges = [&make_lamb_triangles](Mesh* mesh) {
			if (!mesh->lamb_edges.empty()) return;
			make_lamb_triangles(mesh);
			// add all the edges of the triangles:
			mesh->lamb_edges.reserve(mesh->lamb_triangles.size() * 2);
			for (uint32_t i = 0; i + 2 < mesh->lamb_triangles.size(); i += 3) {
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 0]);
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 1]);
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 1]);
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 2]);
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 2]);
				mesh->lamb_edges.emplace_back(mesh->lamb_triangles[i + 0]);
			}
		};

		// helper that pulls out the inverse transpose of the upper-left 3x3 of a Mat4:
		auto normal_to_world = [](Mat4 const& l2w) {
			return Mat4(l2w[0][0], l2w[0][1], l2w[0][2], 0.0f, l2w[1][0], l2w[1][1], l2w[1][2],
			            0.0f, l2w[2][0], l2w[2][1], l2w[2][2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f)
			    .T()
			    .inverse();
		};

		// parameters structure that will be re-used over lambertian program pipeline runs:
		Programs::Lambertian::Parameters parameters;

		// set the lighting parameters:
		parameters.sun_energy = sun_energy;
		parameters.sun_direction = sun_direction;

		parameters.sky_energy = sky_energy;
		parameters.ground_energy = ground_energy;
		parameters.sky_direction = sky_direction;

		uint32_t done = 0;
		uint32_t count = static_cast<uint32_t>(instances.size());

		for (auto const& instance : instances) {
			if (quit) break;
			if (instance.material->type == Material::Type::Lambertian) {
				parameters.local_to_clip = world_to_clip * instance.local_to_world;
				parameters.normal_to_world = normal_to_world(instance.local_to_world);

				parameters.image = instance.material->image;
				// std::string desc = "Rasterizing instance '" + instance.name + "'"; //DEBUG
				if (instance.draw_style == DrawStyle::Wireframe) {
					make_lamb_edges(instance.mesh);
					// desc += " as wireframe (" +
					// std::to_string(instance.mesh->source.faces.size()) + " faces converted to " +
					// std::to_string(instance.mesh->lamb_edges.size()/2) + " lines)"; //DEBUG
					// info("%s",desc.c_str()); //DEBUG
					if (instance.blend_style == BlendStyle::Replace) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Lines_Replace_Always_Pipeline::run(instance.mesh->lamb_edges,
							                                              parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Lines_Replace_Never_Pipeline::run(instance.mesh->lamb_edges,
							                                             parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Lines_Replace_Less_Pipeline::run(instance.mesh->lamb_edges,
							                                            parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Add) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Lines_Add_Always_Pipeline::run(instance.mesh->lamb_edges,
							                                          parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Lines_Add_Never_Pipeline::run(instance.mesh->lamb_edges,
							                                         parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Lines_Add_Less_Pipeline::run(instance.mesh->lamb_edges,
							                                        parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Over) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Lines_Over_Always_Pipeline::run(instance.mesh->lamb_edges,
							                                           parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Lines_Over_Never_Pipeline::run(instance.mesh->lamb_edges,
							                                          parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Lines_Over_Less_Pipeline::run(instance.mesh->lamb_edges,
							                                         parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else {
						// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown blend
						// style)"; //DEBUG
					}
				} else if (instance.draw_style == DrawStyle::Flat) {
					make_lamb_triangles(instance.mesh);
					// desc += " as wireframe (" +
					// std::to_string(instance.mesh->source.faces.size()) + " faces converted to " +
					// std::to_string(instance.mesh->lamb_edges.size()/2) + " lines)"; //DEBUG
					// info("%s",desc.c_str()); //DEBUG
					if (instance.blend_style == BlendStyle::Replace) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Replace_Always_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Replace_Never_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Replace_Less_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Add) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Add_Always_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Add_Never_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Add_Less_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Over) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Over_Always_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Over_Never_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Over_Less_Flat_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else {
						// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown blend
						// style)"; //DEBUG
					}
				} else if (instance.draw_style == DrawStyle::Smooth) {
					make_lamb_triangles(instance.mesh);
					// desc += " as wireframe (" +
					// std::to_string(instance.mesh->source.faces.size()) + " faces converted to " +
					// std::to_string(instance.mesh->lamb_edges.size()/2) + " lines)"; //DEBUG
					// info("%s",desc.c_str()); //DEBUG
					if (instance.blend_style == BlendStyle::Replace) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Replace_Always_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Replace_Never_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Replace_Less_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Add) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Add_Always_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Add_Never_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Add_Less_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Over) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Over_Always_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Over_Never_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Over_Less_Smooth_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else {
						// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown blend
						// style)"; //DEBUG
					}
				} else if (instance.draw_style == DrawStyle::Correct) {
					make_lamb_triangles(instance.mesh);
					// desc += " as wireframe (" +
					// std::to_string(instance.mesh->source.faces.size()) + " faces converted to " +
					// std::to_string(instance.mesh->lamb_edges.size()/2) + " lines)"; //DEBUG
					// info("%s",desc.c_str()); //DEBUG
					if (instance.blend_style == BlendStyle::Replace) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Replace_Always_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Replace_Never_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Replace_Less_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Add) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Add_Always_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Add_Never_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Add_Less_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else if (instance.blend_style == BlendStyle::Over) {
						if (instance.depth_style == DepthStyle::Always) {
							Lambertian_Triangles_Over_Always_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Never) {
							Lambertian_Triangles_Over_Never_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else if (instance.depth_style == DepthStyle::Less) {
							Lambertian_Triangles_Over_Less_Correct_Pipeline::run(
								instance.mesh->lamb_triangles, parameters, &framebuffer);
						} else {
							// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown
							// depth style)"; //DEBUG
						}
					} else {
						// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown blend
						// style)"; //DEBUG
					}
				} else {
					// desc = "!!!SKIPPING!!! instance '" + instance.name + "' (unknown draw
					// style)"; //DEBUG
				}
			} else {
				// TODO: other material types!
			}
			done += 1;
			report_fn(std::make_pair(done / float(count), framebuffer.resolve_colors()));
		}
		report_fn(std::make_pair(1.0f, framebuffer.resolve_colors()));
	}
};

Rasterizer::Rasterizer(Scene const& scene, Instance::Camera const& camera,
                       std::function<void(Render_Report)>&& report_fn) {

	// copy data into the rasterization job:
	job = std::make_unique<RasterJob>(scene, camera, std::move(report_fn));

	// get pointer to output framebuffer (for later use):
	framebuffer = &job->framebuffer;

	// start the rasterization job (asynchronously):
	future = std::async(
		std::launch::async,
		[](RasterJob* job, float* completion_time) {
			Timer timer;
			job->run();
			*completion_time = timer.s();
		},
		job.get(), &completion_time);
}

Rasterizer::~Rasterizer() {
	cancel();
}

void Rasterizer::cancel() {
	signal(); // tell it to quit
	wait();   // wait for it to finish
}

void Rasterizer::signal() {
	// if currently running, tell it to quit (but don't wait on it):
	if (future.valid()) job->quit = true;
}

void Rasterizer::wait() {
	if (future.valid()) future.get();
}

bool Rasterizer::in_progress() {
	if (future.valid()) {
		if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			future.get();
		}
	}
	return future.valid();
}
