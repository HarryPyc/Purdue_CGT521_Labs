#include <vector>
#include <glm/glm.hpp>
#include "Surf.h"
#include <glm/gtc/matrix_transform.hpp>

static const int N = 50;
static const int NUM_VERTICES = N * N;
static const int RESTART = 9999;
//The sinc surface
static glm::vec3 surf(int i, int j)
{
	const float center = 0.5f*N;
	const float xy_scale = 20.0f / N;
	const float z_scale = 10.0f;

	float x = xy_scale * (i - center);
	float y = xy_scale * (j - center);

	float r = sqrt(x*x + y * y);
	float z = 1.0f;

	if (r != 0.0f)
	{
		z = sin(r) / r;
	}
	z = z * z_scale;

	return 0.05f*glm::vec3(x, y, z);
}

//Sinc surface normals
static glm::vec3 normal(int i, int j)
{
	glm::vec3 du = surf(i + 1, j) - surf(i - 1, j);
	glm::vec3 dv = surf(i, j + 1) - surf(i, j - 1);
	return glm::normalize(glm::cross(du, dv));
}

inline glm::vec2 tex_coord(int i, int j) {
	return glm::vec2(float(i) / N, float(j) / N);  //u, v
}

GLuint create_surf_vbo()
{
	float vertices[NUM_VERTICES * 8];
	int i = 0, j = 0, index = 0;
	for (int row = 0; row < NUM_VERTICES * 8; row += 8) {
		glm::vec3 pos = surf(i, j);
		glm::vec3 n = normal(i, j);
		glm::vec2 uv = tex_coord(i, j);
		index++;
		j = index / N;
		i = index % N;
		//interleaved vbo
		//vertex position
		vertices[row] = pos.x;
		vertices[row + 1] = pos.y;
		vertices[row + 2] = pos.z;
		//normal vector
		vertices[row + 3] = n.x;
		vertices[row + 4] = n.y;
		vertices[row + 5] = n.z;
		//texture coordinates
		vertices[row + 6] = uv.x;
		vertices[row + 7] = uv.y;
	}

	GLuint vbo;
	glGenBuffers(1, &vbo); //Generate vbo to hold vertex attributes for triangle.

	glBindBuffer(GL_ARRAY_BUFFER, vbo); //Specify the buffer where vertex attribute data is stored.

	//Upload from main memory to gpu memory.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

	return vbo;
}

//instanced transform matrices
GLuint create_M_vbo() {
	std::vector<glm::mat4> Matrices;
	glm::mat4 M = glm::mat4(1.0);
	Matrices.push_back(glm::translate(M, glm::vec3(0, 0, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(-1, 0, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(+1, 0, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(0, -1, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(0, +1, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(-1, +1, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(-1, -1, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(+1, +1, 0)));
	Matrices.push_back(glm::translate(M, glm::vec3(+1, -1, 0)));

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, Matrices.size() * sizeof(glm::mat4), &Matrices[0], GL_STATIC_DRAW);

	return vbo;
}

//instanced color
GLuint create_color_vbo() {
	std::vector<glm::vec3> color_buffer;
	for (int i = 0; i < 9; i++) {
		color_buffer.push_back(glm::vec3(abs(sin(i)), abs(cos(2 * i)), abs(sin(3 * i))));
	}
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, color_buffer.size() * sizeof(glm::vec3), &color_buffer[0], GL_STATIC_DRAW);
	return vbo;
}


GLuint create_surf_ebo() {
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	int indices[NUM_VERTICES + 50];
	int index = 0;
	for (int j = 0; j < N; j++) {
		for (int i = 0; i < N; i++) {
			if (i % 2 == 0)
				indices[index++] = i + j * N;
			else
				indices[index++] = i - 1 + (j + 1) * N;
		}
		indices[index++] = RESTART;//insert primitive restart index
	}
	
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);
	glPrimitiveRestartIndex(RESTART);
	return ebo;
}


GLuint create_surf_vao()
{
	GLuint vao;

	//Generate vao id to hold the mapping from attrib variables in shader to memory locations in vbo
	glGenVertexArrays(1, &vao);

	//Binding vao means that bindbuffer, enablevertexattribarray and vertexattribpointer state will be remembered by vao
	glBindVertexArray(vao);

	GLuint vbo = create_surf_vbo();
	GLuint ebo = create_surf_ebo();

	const GLuint pos_loc = 0;
	const GLuint normal_loc = 1;
	const GLuint tex_loc = 2;
	glEnableVertexAttribArray(pos_loc); //Enable the position attribute.
	glEnableVertexAttribArray(normal_loc);
	glEnableVertexAttribArray(tex_loc);
	//Tell opengl how to get the attribute values out of the vbo (stride and offset).
	glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
	glVertexAttribPointer(normal_loc, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(tex_loc, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	
	//bind a new vbo
	GLuint Mvbo = create_M_vbo();

	const GLuint M_loc = 3;
	glEnableVertexAttribArray(M_loc);
	int pos1 = M_loc + 0;
	int pos2 = M_loc + 1;
	int pos3 = M_loc + 2;
	int pos4 = M_loc + 3;
	glEnableVertexAttribArray(pos1);
	glEnableVertexAttribArray(pos2);
	glEnableVertexAttribArray(pos3);
	glEnableVertexAttribArray(pos4);
	//Matrix can only be passed as four vec4
	glVertexAttribPointer(pos1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(0));
	glVertexAttribPointer(pos2, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * (4)));
	glVertexAttribPointer(pos3, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * (8)));
	glVertexAttribPointer(pos4, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(sizeof(float) * (12)));
	glVertexAttribDivisor(pos1, 1);
	glVertexAttribDivisor(pos2, 1);
	glVertexAttribDivisor(pos3, 1);
	glVertexAttribDivisor(pos4, 1);

	GLuint Cvbo = create_color_vbo();

	const int color_pos = 7;
	glEnableVertexAttribArray(color_pos);

	glVertexAttribPointer(color_pos, 3, GL_FLOAT, false, 0, 0);
	glVertexAttribDivisor(color_pos, 1);
	glBindVertexArray(0); //unbind the vao

	return vao;
}

//Draw the set of points on the surface
void draw_surf(GLuint vao)
{
	glBindVertexArray(vao);
	
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, NUM_VERTICES, GL_UNSIGNED_INT, 0, 9);
	
}