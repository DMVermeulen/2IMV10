#pragma once

#include<vector>
#include<string>
#include"structures.h"
#include"glad/glad.h"

class Instance {
public:
	Instance(std::string path, float radius, int nTris);
	~Instance();
	int getNumberVertices();
	int getNumberIndices();
	int getNumberTriangles();
	std::vector<Vertex>& getVertices();
	std::vector<uint32_t>& getIndices();
	void setVisible(bool visible);
	void draw();
private:
	std::vector<Tube>tubes;
	std::vector<Vertex>vertices;
	std::vector<uint32_t>indices;
	bool visible=true;
	GLuint VAO, VBO, IBO;

	void loadFromTCK(std::string path);
	void createTriangles(float radius, int nTris);
	void initVertexIndiceBuffer();
	std::vector<std::vector<glm::vec3> > readTCK(const std::string& filename, int offset, int count);

};