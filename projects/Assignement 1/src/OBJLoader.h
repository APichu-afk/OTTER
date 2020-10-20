#pragma once

#include "MeshBuilder.h"
#include "MeshFactory.h"

class OBJLoader
{
public:
	static VertexArrayObject::sptr LoadFile(const std::string& filename);
protected:
	OBJLoader() = default;
	~OBJLoader() = default;
};