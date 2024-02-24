#include"Instance.h"
#include<fstream>
#include"glm/glm.hpp"
#include <glm/gtc/constants.hpp>

Instance::Instance(std::string path, float radius, int nTris) {
	loadFromTCK(path);
	createTriangles(radius, nTris);
	initVertexIndiceBuffer();

	////debug
	//vertices = { Vertex{glm::vec3(0.5f,  0.5f, 0.0f)},Vertex{glm::vec3(0.5f, -0.5f, 0.0f)},Vertex{glm::vec3(-0.5f, -0.5f, 0.0f)},Vertex{glm::vec3(-0.5f,  0.5f, 0.0f)} };
	//indices = { 0,1,3,1,2,3 };
	//initVertexIndiceBuffer();
}

Instance::~Instance() {

}

int Instance::getNumberVertices(){
	return vertices.size();
}

int Instance::getNumberIndices() {
	return indices.size();
}

int Instance::getNumberTriangles() {
	return indices.size() / 3;
}

std::vector<Vertex>& Instance::getVertices() {
	return vertices;
}

std::vector<uint32_t>& Instance::getIndices() {
	return indices;
}

std::vector<std::vector<glm::vec3> > Instance::readTCK(const std::string& filename, int offset, int count) {
	std::ifstream file(filename, std::ios::binary);
	// Read track data from file
	if (!file.is_open()) {
		std::runtime_error("failed to opening file!");
	}

	// Seek to the specified offset
	file.seekg(offset, std::ios::beg);

	std::vector<std::vector<glm::vec3>> tracks;
	std::vector<glm::vec3> currentStreamline;
	while (!file.eof()) {
		float x, y, z;
		file.read(reinterpret_cast<char*>(&x), sizeof(float));
		file.read(reinterpret_cast<char*>(&y), sizeof(float));
		file.read(reinterpret_cast<char*>(&z), sizeof(float));

		// Check if x is NaN, indicating the start of a new streamline
		if (std::isnan(x)) {
			if (!currentStreamline.empty()) {
				tracks.push_back(currentStreamline);
				currentStreamline.clear();
			}
		}
		else if (std::isinf(x)) {
			// Check if x is Inf, indicating the end of the file
			break;
		}
		else {
			// Add the point to the current streamline
			currentStreamline.push_back(glm::vec3(x, y, z));
		}
	}
	file.close();
	return tracks;
}

void Instance::loadFromTCK(std::string path) {
	//load data from tck
	//std::string filename = "C:\\Users\\zzc_c\\Downloads\\whole_brain.tck"; 
	int OFFSET = 107;
	int count = 144000;
	std::vector<std::vector<glm::vec3> >tracks = readTCK(path, OFFSET, count);
	//init vertices from loaded data
	int offset = 0;
	for (int i = 0; i < tracks.size(); i += 50) {
		for (int j = 0; j < tracks[i].size() - 1; j++) {
			tubes.push_back(Tube{ tracks[i][j] ,tracks[i][j + 1], });
		}
	}
}

void Instance::createTriangles(float radius, int nTris) {
	float delta = 2 * glm::pi<float>() / nTris;
	int offset = 0;
	for (int i = 0; i < tubes.size(); i++) {
		Tube tube = tubes[i];
		glm::vec3 up = glm::vec3(0, 0, 1);
		glm::vec3 ez = glm::normalize(tube.p2 - tube.p1);
		glm::vec3 ey = cross(up, ez);
		glm::vec3 ex = cross(ez, ey);

		glm::mat3 R = glm::mat3(ex, ey, ez);
		glm::vec3 T = tube.p1;
		float leng = sqrt(glm::dot(tubes[i].p2 - tubes[i].p1, tubes[i].p2 - tubes[i].p1));
		for (int k = 0; k < nTris; k++) {
			float theta = delta * k;
			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),leng) + T,0) });
		}
		for (int k = 0; k < nTris; k++) {
			float theta = delta * k;
			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),0) + T,0) });
		}
		for (int k = 0; k < nTris; k++) {
			indices.push_back(offset + k);
			indices.push_back(offset + nTris + k);
			indices.push_back(offset + (k + 1) % nTris);

			indices.push_back(offset + (k + 1) % nTris);
			indices.push_back(offset + nTris + k);
			indices.push_back(offset + nTris + (k + 1) % nTris);
		}
		offset += 2 * nTris;
	}

	////debug
	//vertices = { Vertex{glm::vec3(-0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, 0.5f, -10)} };
	//indices = { 0,1,2 };
}

void Instance::setVisible(bool _visible) {
	visible = _visible;
}

void Instance::initVertexIndiceBuffer() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &IBO);

	glBindVertexArray(VAO);

	//VBO stores vertex positions, possibly extended to store more attributes
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//IBO stores indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Instance::draw() {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}