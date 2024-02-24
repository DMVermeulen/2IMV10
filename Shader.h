#pragma once
#include"glad/glad.h"
#include<string>
class Shader {

public:
	Shader(std::string vShaderPath, std::string fShaderPath);
	~Shader();

	void enable();
	void disable();
	GLuint getProgramId();

private:
	const GLchar* vShaderCode;
	const GLchar* fShaderCode;
	GLuint programId;

	void load(std::string vShaderPath, std::string fShaderPath);

};