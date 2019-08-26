#pragma once
#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <vector>
#include "math.h"

unsigned int load_shader(const char* vertex, const char* fragment, const char* geometry = nullptr)
{
	unsigned int program = glCreateProgram();
	std::vector<unsigned int> shaders;
	
	auto attach_shader = [&](const char* source, GLenum shader_type)
	{
		unsigned int shader = glCreateShader(shader_type);

		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);
		// Check for errors
		int success;
		char info_log[1024];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, info_log);
			std::cout << info_log << std::endl;
		}

		glAttachShader(program, shader);
		shaders.push_back(shader);
	};

	attach_shader(vertex, GL_VERTEX_SHADER);
	attach_shader(fragment, GL_FRAGMENT_SHADER);
	if (geometry)
	{
		attach_shader(geometry, GL_GEOMETRY_SHADER);
	}

	glLinkProgram(program);
	// Check for errors
	int success;
	char info_log[1024];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 1024, NULL, info_log);
		std::cout << info_log << std::endl;
	}

	for (unsigned int shader : shaders) 
	{
		glDeleteShader(shader);
	}

	return program;
}

unsigned int load_texture(unsigned char* data, int width, int height, int components)
{
	unsigned int texture;
	glGenTextures(1, &texture);

	GLenum format;
	if (components == 1)
		format = GL_RED;
	else if (components == 3)
		format = GL_RGB;
	else if (components == 4)
		format = GL_RGBA;

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texture;
}

unsigned int load_render_target(unsigned int texture, int width, int height)
{
	unsigned int rbo;
	glGenBuffers(1, &rbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return rbo;
}

struct Vertex
{
	float position[2];
	float uv[2];
};

struct Mesh
{
	unsigned int VBO;
	unsigned int VAO;
	unsigned int EBO;
};

void load_mesh(Mesh& mesh, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
	if (mesh.VBO == 0) //mesh hasn't been loaded before
	{
		glGenBuffers(1, &mesh.VBO);
		glGenVertexArrays(1, &mesh.VAO);
		glGenBuffers(1, &mesh.EBO);
	}

	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBindVertexArray(mesh.VAO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_DYNAMIC_DRAW);

	glBindVertexArray(0);
}

struct LineMesh
{
	unsigned int VBO;
	unsigned int VAO;
	unsigned int length;
};
void load_line(LineMesh& mesh, std::vector<Vector>& line)
{
	if (mesh.VBO == 0) //mesh hasn't been loaded before
	{
		glGenBuffers(1, &mesh.VBO);
		glGenVertexArrays(1, &mesh.VAO);
	}

	mesh.length = line.size();

	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBindVertexArray(mesh.VAO);
	glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(Vector), &line[0], GL_DYNAMIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector), (void*)0);
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
}

void glDebugOutput(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	void *userParam)
{
	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

void glfwErrorOutput(int error, const char* message)
{
	std::cout << "---------------" << std::endl;
	std::cout << "GLFW ERROR :" << error << std::endl << message << std::endl;
}

struct TriangulatedLine
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

void line_to_tris(std::vector<Vector> line, float width, TriangulatedLine& result)
{
	result.vertices.reserve(result.vertices.size() + line.size() * 2);
	result.indices.reserve(result.indices.size() + line.size() * 2 * 6);

	width /= 2.0;
	Vector direction;
	float length = 0;

	for (int i = 0; i < line.size(); i++)
	{
		Vector left;
		Vector right;

		if (i != 0)
		{
			length += (line[i] - line[i - 1]).length();
		}

		if (i == 0) //first element
		{
			direction = (line[i + 1] - line[i]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else if (i == line.size() - 1) //last element
		{
			direction = (line[i] - line[i - 1]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else
		{
			Vector previous_direction = direction;
			Vector current_direction = (line[i + 1] - line[i]).normalized();
			direction = current_direction;

			Vector previous_side_direction = previous_direction.rotated(-90 * DEG2RAD);
			Vector previous_left = line[i - 1] + previous_side_direction * width;
			Vector previous_right = line[i - 1] - previous_side_direction * width;

			Vector current_side_direction = current_direction.rotated(-90 * DEG2RAD);
			Vector next_left = line[i + 1] + current_side_direction * width;
			Vector next_right = line[i + 1] - current_side_direction * width;

			bool intersect = line_intersection(previous_left, previous_left + previous_direction, next_left, next_left - current_direction, left);
			intersect = intersect && line_intersection(previous_right, previous_right + previous_direction, next_right, next_right - current_direction, right);

			if (!intersect) //they are parallel
			{
				left = line[i] + current_side_direction * width;
				right = line[i] - current_side_direction * width;
			}
		}

		Vertex vertex_left = { {left.x, left.y }, {length, 0} };
		Vertex vertex_right = { {right.x, right.y }, {length, 1} };
		result.vertices.insert(result.vertices.end(), { vertex_left, vertex_right });

		if (i != 0)
		{
			//first vertex index of the new quad
			unsigned int v = result.vertices.size() - 4;
			result.indices.insert(result.indices.end(), { v, v + 2, v + 3, v, v + 3, v + 1 });
		}

	}
}

void line_to_debug_tris(std::vector<Vector> line, float width, TriangulatedLine& result)
{
	result.vertices.reserve(result.vertices.size() + line.size() * 4);
	result.indices.reserve(result.indices.size() + line.size() * 4 * 6);

	width /= 2.0;
	Vector direction;
	float length = 0;

	for (int i = 0; i < line.size(); i++)
	{
		Vector left;
		Vector right;

		if (i != 0)
		{
			length += (line[i] - line[i - 1]).length();
		}

		if (i == 0) //first element
		{
			direction = (line[i + 1] - line[i]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else if (i == line.size() - 1) //last element
		{
			direction = (line[i] - line[i - 1]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else
		{
			Vector previous_direction = direction;
			Vector current_direction = (line[i + 1] - line[i]).normalized();
			direction = current_direction;

			Vector previous_side_direction = previous_direction.rotated(-90 * DEG2RAD);
			Vector previous_left = line[i - 1] + previous_side_direction * width;
			Vector previous_right = line[i - 1] - previous_side_direction * width;

			Vector current_side_direction = current_direction.rotated(-90 * DEG2RAD);
			Vector next_left = line[i + 1] + current_side_direction * width;
			Vector next_right = line[i + 1] - current_side_direction * width;

			bool intersect = line_intersection(previous_left, previous_left + previous_direction, next_left, next_left - current_direction, left);
			intersect = intersect && line_intersection(previous_right, previous_right + previous_direction, next_right, next_right - current_direction, right);

			if (!intersect) //they are parallel
			{
				left = line[i] + current_side_direction * width;
				right = line[i] - current_side_direction * width;
			}
		}

		Vector a = left - direction;
		Vector b = right - direction;
		Vector c = left + direction;
		Vector d = right + direction;

		Vertex vertex_a = { {a.x, a.y }, {0, 0} };
		Vertex vertex_b = { {b.x, b.y }, {0, 1} };
		Vertex vertex_c = { {c.x, c.y }, {1, 0} };
		Vertex vertex_d = { {d.x, d.y }, {1, 1} };

		result.vertices.insert(result.vertices.end(), { vertex_a, vertex_b, vertex_c, vertex_d });

		//first vertex index of the new quad
		unsigned int v = result.vertices.size() - 4;
		result.indices.insert(result.indices.end(), { v, v + 2, v + 3, v, v + 3, v + 1 });
	}
}
