#include "OBJLoader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

// Borrowed from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#pragma region String Trimming

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

#pragma endregion 

VertexArrayObject::sptr OBJLoader::LoadFile(const std::string& filename)
{
	std::vector< unsigned int > verIndices, uvIndices, normIndices;
	std::vector< glm::vec3 > temp_ver;
	std::vector< glm::vec2 > temp_vt;
	std::vector< glm::vec3 > temp_vn;

	std::ifstream fileName;
	fileName.open(filename, std::ios::binary);

	// If our file fails to open, we will throw an error
	if (!fileName) {
		throw std::runtime_error("file not opening");
	}

	MeshBuilder<VertexPosNormTexCol> object;
	std::string fileLine;

	while (std::getline(fileName, fileLine))
	{
		trim(fileLine);
		if (fileLine.substr(0, 1) == "#")
		{
			// Comment, do nothing
		}
		else if (fileLine.substr(0, 2) == "v ")
		{
			std::istringstream ss = std::istringstream(fileLine.substr(2));
			glm::vec3 ver;
			ss >> ver.x >> ver.y >> ver.z;
			temp_ver.push_back(ver);

		}
		else if (fileLine.substr(0, 3) == "vt ")
		{
			std::istringstream ss = std::istringstream(fileLine.substr(3));
			glm::vec2 vt;
			ss >> vt.x >> vt.y;
			temp_vt.push_back(vt);

		}
		else if (fileLine.substr(0, 3) == "vn ")
		{
			std::istringstream ss = std::istringstream(fileLine.substr(3));
			glm::vec3 vn;
			ss >> vn.x >> vn.y >> vn.z;
			temp_vn.push_back(vn);
		}
		else if (fileLine.substr(0, 2) == "f ")
		{
			std::string fileLineSubstr = fileLine.substr(2);
			std::string fileLineWithoutSlashes = fileLineSubstr;

			// Remove all of the slashes and replace them with spaces
			for (int i = 0; i < fileLineWithoutSlashes.length(); i++)
			{
				if (fileLineWithoutSlashes[i] == '/')
					fileLineWithoutSlashes[i] = ' ';
			}

			std::istringstream ss = std::istringstream(fileLineWithoutSlashes);
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			ss >> vertexIndex[0] >> uvIndex[0] >> normalIndex[0] >> vertexIndex[1] >> uvIndex[1] >> normalIndex[1] >> vertexIndex[2] >> uvIndex[2] >> normalIndex[2];
			verIndices.push_back(vertexIndex[0]);
			verIndices.push_back(vertexIndex[1]);
			verIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normIndices.push_back(normalIndex[0]);
			normIndices.push_back(normalIndex[1]);
			normIndices.push_back(normalIndex[2]);
		}
	}
	for (unsigned int i = 0; i < verIndices.size(); i++) {
		unsigned int vertexIndex = verIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normIndex = normIndices[i];
		glm::vec3 vertex = temp_ver[vertexIndex - 1];
		glm::vec2 uv = temp_vt[uvIndex - 1];
		glm::vec3 normal = temp_vn[normIndex - 1];
		
		object.AddVertex(vertex, normal, uv, glm::vec4(1));
		object.AddIndex(i);
	}
	return object.Bake();
}
