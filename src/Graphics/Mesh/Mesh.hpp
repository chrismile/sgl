/*!
 * Mesh.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_MESH_MESH_HPP_
#define GRAPHICS_MESH_MESH_HPP_

#include "SubMesh.hpp"
#include <vector>

namespace sgl {

class DLL_OBJECT Mesh
{
public:
	void render();
	bool loadFromXML(const char *filename);
	inline const AABB3 &getAABB() const { return aabb; }

	//! Call these functions to create a mesh manually
	void addSubMesh(SubMeshPtr &submesh);
	void finalizeManualMesh();

private:
	void computeAABB();
	std::vector<SubMeshPtr> submeshes;
	AABB3 aabb;
};

typedef boost::shared_ptr<Mesh> MeshPtr;

}

/*! GRAPHICS_MESH_MESH_HPP_ */
#endif
