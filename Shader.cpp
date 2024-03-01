#include "Shader.h"
#include <fstream>
#include <sstream>
#include<iostream>

Shader::Shader(std::string vPath, std::string fPath) {
	load(vPath, fPath);
}

Shader::~Shader() {

}

void Shader::load(std::string vPath, std::string fPath) {
	std::string vertexCode;
	std::string fragmentCode;

	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	vShaderFile.exceptions(std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::badbit);
	try {
		vShaderFile.open(vPath);
		fShaderFile.open(fPath);
		std::stringstream vShaderStream, fShaderStream;

		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();

		vShaderFile.close();
		fShaderFile.close();

		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e) {
		std::cout << "Load shader failed!" << std::endl;
	}

	vShaderCode = reinterpret_cast<const GLchar*>(vertexCode.c_str());
	fShaderCode = reinterpret_cast<const GLchar*>(fragmentCode.c_str());

	GLint vShaderId, fShaderId;
	GLint success;
	vShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShaderId, 1, &vShaderCode, NULL);
	glCompileShader(vShaderId);

	GLchar infoLog[512];
	glGetShaderiv(vShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(vShaderId, 512, NULL, infoLog);
		std::cout << "Vertex shader compilation failed!"<<infoLog << std::endl;
	}

	fShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShaderId, 1, &fShaderCode, NULL);
	glCompileShader(fShaderId);

	glGetShaderiv(fShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(fShaderId, 512, NULL, infoLog);
		std::cout << "Fragment shader compilation failed!" << infoLog << std::endl;
	}

	programId = glCreateProgram();
	glAttachShader(programId, vShaderId);
	glAttachShader(programId, fShaderId);
	glLinkProgram(programId);


	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programId, 512, NULL, infoLog);
		std::cout << "Shader linking failed! " << infoLog << std::endl;
	}
	glDeleteShader(vShaderId);
	glDeleteShader(fShaderId);

}

void Shader::enable() {
	glUseProgram(programId);
}

void Shader::disable() {
	glUseProgram(0);
}

GLuint Shader::getProgramId() {
	return programId;
}

void Shader::setInt(const std::string &name, int value)
{
	glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value)
{
	glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value)
{
	glUniform3fv(glGetUniformLocation(programId, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value)
{
	glUniform4fv(glGetUniformLocation(programId, name.c_str()), 1, &value[0]);
}

void Shader::setMat3(const std::string &name, const glm::mat3 &mat)
{
	glUniformMatrix3fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &mat)
{
	glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}