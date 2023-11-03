
#include "bvh.h"
#include "aggregate.h"
#include "instance.h"
#include "tri_mesh.h"

#include <stack>

#include "./test.h"

namespace PT {

struct BVHBuildData {
	BVHBuildData(size_t start, size_t range, size_t dst) : start(start), range(range), node(dst) {
	}
	size_t start; ///< start index into the primitive array
	size_t range; ///< range of index into the primitive array
	size_t node;  ///< address to update
};

struct SAHBucketData {
	BBox bb;          ///< bbox of all primitives
	size_t num_prims; ///< number of primitives in the bucket
};

template<typename Primitive>
void BVH<Primitive>::build(std::vector<Primitive>&& prims, size_t max_leaf_size) {
	//A3T3 - build a bvh

	// Keep these
    nodes.clear();
    primitives = std::move(prims);

    // Construct a BVH from the given vector of primitives and maximum leaf
    // size configuration.

	const size_t BUCKETS_NUM = 8;
	std::stack<size_t> nodes_stack;
	BBox scene_box;
	for(auto& prim : primitives) scene_box.enclose(prim.bbox());
	root_idx = new_node(scene_box, 0, primitives.size());
	nodes_stack.push(root_idx);

	while(!nodes_stack.empty())
	{
		size_t idx = nodes_stack.top();
		auto& node = nodes[idx];
		nodes_stack.pop();

		if(node.size <= max_leaf_size) continue;

		size_t node_end = node.start + node.size;
		int32_t best_axis = -1;
		float best_coord = -1;
		float best_score = std::numeric_limits<float>::max();

		for(int32_t axis = 0; axis < 3; ++axis)
		{
			std::array<SAHBucketData, BUCKETS_NUM> buckets{};
			float length = node.bbox.max[axis] - node.bbox.min[axis];

			for(size_t i = node.start; i < node_end; ++i)
			{
				size_t bucket_idx = static_cast<size_t>((primitives[i].bbox().center()[axis]
				- node.bbox.min[axis]) * BUCKETS_NUM / length);
				bucket_idx = std::min(bucket_idx, BUCKETS_NUM - 1);
				buckets[bucket_idx].bb.enclose(primitives[i].bbox());
				++buckets[bucket_idx].num_prims;
			}
			
			BBox box_l, box_r;
			size_t size_l = 0, size_r = 0;

			for(int32_t i = 0; i < BUCKETS_NUM - 1; ++i)
			{
				box_l.enclose(buckets[i].bb);
				size_l += buckets[i].num_prims;
				box_r.reset(), size_r = 0;
				for(int32_t j = i + 1; j < BUCKETS_NUM; ++j)
				{
					box_r.enclose(buckets[j].bb);
					size_r += buckets[j].num_prims;
				}
				float cur_score = box_l.surface_area() * size_l + box_r.surface_area() * size_r;
				if(cur_score >= best_score) continue;
				best_axis = axis, best_score = cur_score;
				best_coord = node.bbox.min[axis] + length / BUCKETS_NUM * (i + 1.f);
			}
		}

		BBox box_l, box_r;
		size_t part_idx = std::partition(
			primitives.begin() + node.start, 
			primitives.begin() + node_end, 
			[axis = best_axis, coord = best_coord](const Primitive& p) {
				return p.bbox().center()[axis] < coord;
			}
			) - primitives.begin();
		if(part_idx == node.start || part_idx == node_end) continue;

		for(size_t i = node.start; i < part_idx; ++i)
			box_l.enclose(primitives[i].bbox());
		for(size_t i = part_idx; i < node_end; ++i)
			box_r.enclose(primitives[i].bbox());

		nodes[idx].l = new_node(box_l, node.start, part_idx - node.start);
		nodes[idx].r = new_node(box_r, part_idx, node_end - part_idx);
		nodes_stack.push(nodes[idx].l);
		nodes_stack.push(nodes[idx].r);
	}
}

template<typename Primitive> Trace BVH<Primitive>::hit(const Ray& ray) const {
	//A3T3 - traverse your BVH

    // Implement ray - BVH intersection test. A ray intersects
    // with a BVH aggregate if and only if it intersects a primitive in
    // the BVH that is not an aggregate.

    // The starter code simply iterates through all the primitives.
    // Again, remember you can use hit() on any Primitive value.

    Trace ret;
	if(nodes.empty()) return ret;

	std::stack<size_t> stk;
	stk.push(root_idx);

	while(!stk.empty()) {
		size_t idx = stk.top();
		const auto& node = nodes[idx];
		stk.pop();

		Vec2 times(0.f, FLT_MAX);
		bool is_hit = node.bbox.hit(ray, times);

		if(!is_hit || (ret.hit && ret.distance <= times.x)) continue;

		if(node.is_leaf()) {
			for(size_t i = node.start; i < node.start + node.size; ++i) {
				Trace hit = primitives[i].hit(ray);
				ret = Trace::min(ret, hit);
			}
		} else {
			Vec2 times_l(0.f, FLT_MAX);
			Vec2 times_r(0.f, FLT_MAX);
			bool is_hit_l = nodes[node.l].bbox.hit(ray, times_l);
			bool is_hit_r = nodes[node.r].bbox.hit(ray, times_r);

			if(is_hit_l && is_hit_r) {
				if(times_l.x < times_r.x)
					stk.push(node.l), stk.push(node.r);
				else stk.push(node.r), stk.push(node.l);
			}
			else if(is_hit_l) { stk.push(node.l); }
			else if(is_hit_r) { stk.push(node.r); }
		}
	}

    return ret;
}

template<typename Primitive>
BVH<Primitive>::BVH(std::vector<Primitive>&& prims, size_t max_leaf_size) {
	build(std::move(prims), max_leaf_size);
}

template<typename Primitive> std::vector<Primitive> BVH<Primitive>::destructure() {
	nodes.clear();
	return std::move(primitives);
}

template<typename Primitive>
template<typename P>
typename std::enable_if<std::is_copy_assignable_v<P>, BVH<P>>::type BVH<Primitive>::copy() const {
	BVH<Primitive> ret;
	ret.nodes = nodes;
	ret.primitives = primitives;
	ret.root_idx = root_idx;
	return ret;
}

template<typename Primitive> Vec3 BVH<Primitive>::sample(RNG &rng, Vec3 from) const {
	if (primitives.empty()) return {};
	int32_t n = rng.integer(0, static_cast<int32_t>(primitives.size()));
	return primitives[n].sample(rng, from);
}

template<typename Primitive>
float BVH<Primitive>::pdf(Ray ray, const Mat4& T, const Mat4& iT) const {
	if (primitives.empty()) return 0.0f;
	float ret = 0.0f;
	for (auto& prim : primitives) ret += prim.pdf(ray, T, iT);
	return ret / primitives.size();
}

template<typename Primitive> void BVH<Primitive>::clear() {
	nodes.clear();
	primitives.clear();
}

template<typename Primitive> bool BVH<Primitive>::Node::is_leaf() const {
	// A node is a leaf if l == r, since all interior nodes must have distinct children
	return l == r;
}

template<typename Primitive>
size_t BVH<Primitive>::new_node(BBox box, size_t start, size_t size, size_t l, size_t r) {
	Node n;
	n.bbox = box;
	n.start = start;
	n.size = size;
	n.l = l;
	n.r = r;
	nodes.push_back(n);
	return nodes.size() - 1;
}
 
template<typename Primitive> BBox BVH<Primitive>::bbox() const {
	if(nodes.empty()) return BBox{Vec3{0.0f}, Vec3{0.0f}};
	return nodes[root_idx].bbox;
}

template<typename Primitive> size_t BVH<Primitive>::n_primitives() const {
	return primitives.size();
}

template<typename Primitive>
uint32_t BVH<Primitive>::visualize(GL::Lines& lines, GL::Lines& active, uint32_t level,
                                   const Mat4& trans) const {

	std::stack<std::pair<size_t, uint32_t>> tstack;
	tstack.push({root_idx, 0u});
	uint32_t max_level = 0u;

	if (nodes.empty()) return max_level;

	while (!tstack.empty()) {

		auto [idx, lvl] = tstack.top();
		max_level = std::max(max_level, lvl);
		const Node& node = nodes[idx];
		tstack.pop();

		Spectrum color = lvl == level ? Spectrum(1.0f, 0.0f, 0.0f) : Spectrum(1.0f);
		GL::Lines& add = lvl == level ? active : lines;

		BBox box = node.bbox;
		box.transform(trans);
		Vec3 min = box.min, max = box.max;

		auto edge = [&](Vec3 a, Vec3 b) { add.add(a, b, color); };

		edge(min, Vec3{max.x, min.y, min.z});
		edge(min, Vec3{min.x, max.y, min.z});
		edge(min, Vec3{min.x, min.y, max.z});
		edge(max, Vec3{min.x, max.y, max.z});
		edge(max, Vec3{max.x, min.y, max.z});
		edge(max, Vec3{max.x, max.y, min.z});
		edge(Vec3{min.x, max.y, min.z}, Vec3{max.x, max.y, min.z});
		edge(Vec3{min.x, max.y, min.z}, Vec3{min.x, max.y, max.z});
		edge(Vec3{min.x, min.y, max.z}, Vec3{max.x, min.y, max.z});
		edge(Vec3{min.x, min.y, max.z}, Vec3{min.x, max.y, max.z});
		edge(Vec3{max.x, min.y, min.z}, Vec3{max.x, max.y, min.z});
		edge(Vec3{max.x, min.y, min.z}, Vec3{max.x, min.y, max.z});

		if (!node.is_leaf()) {
			tstack.push({node.l, lvl + 1});
			tstack.push({node.r, lvl + 1});
		} else {
			for (size_t i = node.start; i < node.start + node.size; i++) {
				uint32_t c = primitives[i].visualize(lines, active, level - lvl, trans);
				max_level = std::max(c + lvl, max_level);
			}
		}
	}
	return max_level;
}

template class BVH<Triangle>;
template class BVH<Instance>;
template class BVH<Aggregate>;
template BVH<Triangle> BVH<Triangle>::copy<Triangle>() const;

} // namespace PT
