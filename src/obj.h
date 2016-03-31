#ifndef OBJ_H
#define OBJ_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include "misc.h"

struct v {
	GLfloat x, y, z;
	GLfloat nx, ny, nz;
	GLfloat padding[2];
};

struct vertex {
	std::vector<float> v;
	void normalize() {
		float magnitude = 0.0f;
		for (int i = 0; i < v.size(); i++)
			magnitude += pow(v[i], 2.0f);
		magnitude = sqrt(magnitude);
		for (int i = 0; i < v.size(); i++)
			v[i] /= magnitude;
	}
};

struct face {
	std::vector<int> vertex;
	std::vector<int> texture;
	std::vector<int> normal;
};

class cObj {
  private:
	std::vector<vertex> vertices;
	std::vector<vertex> texcoords;
	std::vector<vertex> normals;
	std::vector<vertex> parameters;
	std::vector<face> faces;
	GLuint vbo_vertices, vbo_indices;
  protected:
  public:
	cObj(std::string filename);
	~cObj();
	void setupBufferObjects();
	void render(GLint vertex, GLint normal);
	void releaseBufferObjects();
};

#endif