#pragma once
#include"glad/glad.h"
#include<string>
#include"glm/glm.hpp"

class Shader {

public:
	Shader(std::string vShaderPath, std::string fShaderPath);
	~Shader();

	void enable();
	void disable();
	GLuint getProgramId();
	//interfaces for setting uniform
	void setInt(const std::string &name, int value);
	void setFloat(const std::string &name, float value);
	void setVec3(const std::string &name, const glm::vec3 &value);
	void setVec4(const std::string &name, const glm::vec4 &value);
	void setMat3(const std::string &name, const glm::mat3 &mat);
	void setMat4(const std::string &name, const glm::mat4 &mat);

private:
	const GLchar* vShaderCode;
	const GLchar* fShaderCode;
	GLuint programId;

	void load(std::string vShaderPath, std::string fShaderPath);

};