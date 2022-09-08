
#pragma once

#include "../lib/mathlib.h"
#include "../platform/gl.h"

class Halfedge_Mesh;

class Indexed_Mesh {
public:
	using Index = uint32_t;
	struct Vert {
		Vec3 pos;
		Vec3 norm;
		Vec2 uv;
		uint32_t id;
	};

	Indexed_Mesh() = default;
	~Indexed_Mesh() = default;

	enum SplitOrAverage {
		SplitEdges, //mesh faces are split along edges so uv and normal data can be perfectly represented
		AverageData, //topology is preserved, but vertex uvs and normals are average of incident corner uvs/normals
	};
	static Indexed_Mesh from_halfedge_mesh(Halfedge_Mesh const &, SplitOrAverage split_or_average);

	Indexed_Mesh(std::vector<Vert>&& vertices, std::vector<Index>&& indices);

	Indexed_Mesh(const Indexed_Mesh& src) = delete;
	Indexed_Mesh(Indexed_Mesh&& src) = default;
	Indexed_Mesh& operator=(const Indexed_Mesh& src) = delete;
	Indexed_Mesh& operator=(Indexed_Mesh&& src) = default;

	std::vector<Vert>& vertices();
	std::vector<Index>& indices();

	const std::vector<Vert>& vertices() const;
	const std::vector<Index>& indices() const;

	uint32_t tris() const;

	Indexed_Mesh copy() const;
	GL::Mesh to_gl() const;

private:
	std::vector<Vert> vs;
	std::vector<Index> is;
};
