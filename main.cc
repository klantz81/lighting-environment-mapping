#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <sstream>
#include <fstream>

#include <GL/glew.h>

#include "glm-0.9.2.6/glm/glm.hpp"
#include "glm-0.9.2.6/glm/gtc/matrix_transform.hpp"
#include "glm-0.9.2.6/glm/gtc/type_ptr.hpp"

#include "src/keyboard.h"
#include "src/joystick.h"
#include "src/glhelper.h"
#include "src/timer.h"
#include "src/misc.h"
#include "src/obj.h"

#define WIDTH  1280
#define HEIGHT 720

int main(int argc, char *argv[]) {
	// input devices
	cKeyboard kb;
	cJoystick js; joystick_position jp[2];
	
	// teapot
//	cObj obj("media/dragon_smooth.obj");
	cObj obj("media/teapot.obj");

	// timers
	cTimer t0; double elapsed0;
	cTimer t1; double elapsed1;

	// buffer for screen grabs
	unsigned char *buffer = new unsigned char[WIDTH * HEIGHT * 4];

	// application is active.. fullscreen flag.. screen grab.. video grab..
	bool active = true, fullscreen = false, grab = false, video = false;

	// setup an opengl context
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_OPENGL);

	// initialize the extension wrangler
	glewInit();

	// for handling events
	SDL_Event event;

	// some output
	std::cout << glGetString(GL_VERSION)<< std::endl;
	std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION)<< std::endl;
	std::cout << glewGetString(GLEW_VERSION)<< std::endl;
	std::cout << glGetString(GL_EXTENSIONS)<< std::endl;

	// set up the cube map texture
	SDL_Surface *xpos = IMG_Load("media/xpos.png");	SDL_Surface *xneg = IMG_Load("media/xneg.png");
	SDL_Surface *ypos = IMG_Load("media/ypos.png");	SDL_Surface *yneg = IMG_Load("media/yneg.png");
	SDL_Surface *zpos = IMG_Load("media/zpos.png");	SDL_Surface *zneg = IMG_Load("media/zneg.png");
	GLuint cubemap_texture;
	setupCubeMap(cubemap_texture, xpos, xneg, ypos, yneg, zpos, zneg);
	SDL_FreeSurface(xneg);	SDL_FreeSurface(xpos);
	SDL_FreeSurface(yneg);	SDL_FreeSurface(ypos);
	SDL_FreeSurface(zneg);	SDL_FreeSurface(zpos);

	// set our viewport, clear color and depth, and enable depth testing
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// load our shaders and compile them.. create a program and link it
	GLuint glProgram, glShaderV, glShaderF;
	createProgram(glProgram, glShaderV, glShaderF, "src/vertex.sh", "src/fragment.sh");
	// grab the pvm matrix and vertex location from our shader program
	GLint PVM    = glGetUniformLocation(glProgram, "PVM");
	GLint vertex = glGetAttribLocation(glProgram, "vertex");

	GLuint glProgram1, glShaderV1, glShaderF1;
	createProgram(glProgram1, glShaderV1, glShaderF1, "src/vertex1.sh", "src/fragment1.sh");
	GLint vertex1         = glGetAttribLocation(glProgram1, "vertex");
	GLint normal1         = glGetAttribLocation(glProgram1, "normal");
	GLint light_position1 = glGetUniformLocation(glProgram1, "light_position");
	GLint Projection1     = glGetUniformLocation(glProgram1, "Projection");
	GLint View1           = glGetUniformLocation(glProgram1, "View");
	GLint Model1          = glGetUniformLocation(glProgram1, "Model");

	obj.setupBufferObjects();

	glm::mat4 Projection = glm::perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f); 
	glm::mat4 View       = glm::mat4(1.0f);
	glm::mat4 Model      = glm::mat4(1.0f);
	glm::mat4 M          = glm::mat4(1.0f);

	// cube vertices for vertex buffer object
	GLfloat cube_vertices[] = {
	  -1.0,  1.0,  1.0,
	  -1.0, -1.0,  1.0,
	   1.0, -1.0,  1.0,
	   1.0,  1.0,  1.0,
	  -1.0,  1.0, -1.0,
	  -1.0, -1.0, -1.0,
	   1.0, -1.0, -1.0,
	   1.0,  1.0, -1.0,
	};
	GLuint vbo_cube_vertices;
	glGenBuffers(1, &vbo_cube_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(vertex);
	glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// cube indices for index buffer object
	GLushort cube_indices[] = {
	  0, 1, 2, 3,
	  3, 2, 6, 7,
	  7, 6, 5, 4,
	  4, 5, 1, 0,
	  0, 3, 7, 4,
	  1, 2, 6, 5,
	};
	GLuint ibo_cube_indices;
	glGenBuffers(1, &ibo_cube_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// rotation angles
	float alpha = 0.0f, beta = 0.0f, gamma = 0.0f;

	while(active) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				active = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_g:
					grab = true;
					break;
				case SDLK_v:
					video ^= true; elapsed1 = 0.0;
					break;
				case SDLK_f:
					fullscreen ^= true;
					screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, (fullscreen ? SDL_FULLSCREEN : 0) | SDL_HWSURFACE | SDL_OPENGL);
					break;
				}
				break;
			}
		}

		// time elapsed since last frame
		elapsed0 = t0.elapsed(true);

		// update frame based on input state
		if (kb.getKeyState(KEY_UP))    alpha += 180.0f*elapsed0;
		if (kb.getKeyState(KEY_DOWN))  alpha -= 180.0f*elapsed0;
		if (kb.getKeyState(KEY_LEFT))  beta  -= 18.0f*elapsed0;
		if (kb.getKeyState(KEY_RIGHT)) beta  += 18.0f*elapsed0;
		jp[0] = js.joystickPosition(0);
		alpha += jp[0].y*elapsed0*180.0f;
		beta  += jp[0].x*elapsed0*180.0f;
		gamma += 45.0f*elapsed0;
		//jp[1] = js.joystickPosition(1); 

		// rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		View  = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, alpha*0.01));
		View  = glm::rotate(View, beta, glm::vec3(0.0f, 1.0f, 0.0f));

		// render teapot
		Model = glm::mat4(1.0f);
		Model = glm::rotate(Model, gamma*.667f, glm::vec3(0.0f,1.0f,0.0f));
		Model = glm::rotate(Model, gamma*.667f, glm::vec3(1.0f,0.0f,0.0f));
		Model = glm::rotate(Model, gamma*.667f, glm::vec3(0.0f,0.0f,1.0f));
		Model = glm::translate(Model, glm::vec3(0.0f,-0.5f,0.0f));
		glm::vec3 light_position = glm::vec3(0.0f, 100.0f, 100.0f);
		glUseProgram(glProgram1);
		glUniform3f(light_position1, light_position.x, light_position.y, light_position.z);
		glUniformMatrix4fv(Projection1, 1, GL_FALSE, glm::value_ptr(Projection));
		glUniformMatrix4fv(View1,       1, GL_FALSE, glm::value_ptr(View));
		glUniformMatrix4fv(Model1,      1, GL_FALSE, glm::value_ptr(Model));
		obj.render(vertex1, normal1);

		// render skybox
		Model = glm::scale(glm::mat4(1.0f), glm::vec3(500,500,500));
		View = glm::mat4(1.0f);
		View = glm::rotate(View, beta, glm::vec3(0.0f, 1.0f, 0.0f));
		M = Projection * View * Model;
		glUseProgram(glProgram);
		glUniformMatrix4fv(PVM, 1, GL_FALSE, glm::value_ptr(M));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
		glEnableVertexAttribArray(vertex);
		glVertexAttribPointer(vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
		glDrawElements(GL_QUADS, sizeof(cube_indices)/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

		SDL_GL_SwapBuffers();

		if (grab) {
			grab = false;
			saveTGA(buffer, WIDTH, HEIGHT);
		}

		if (video) {
			elapsed1 += t1.elapsed(true);
			if (elapsed1 >= 1/24.0) {
				saveTGA(buffer, WIDTH, HEIGHT, true);
				elapsed1 = 0.0;
			}
		}
	}

	obj.releaseBufferObjects();
	
	// release vertex and index buffer object
	glDeleteBuffers(1, &ibo_cube_indices);
	glDeleteBuffers(1, &vbo_cube_vertices);

	// release cube map
	deleteCubeMap(cubemap_texture);

	// detach shaders from program and release
	releaseProgram(glProgram, glShaderV, glShaderF);
	releaseProgram(glProgram1, glShaderV1, glShaderF1);

	SDL_Quit();

	delete[] buffer;

	return 0;
}