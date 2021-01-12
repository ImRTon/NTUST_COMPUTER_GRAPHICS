/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <tuple>
#include <utility>
#include <Fl/fl.h>
#define _USE_MATH_DEFINES
#include <math.h>
using std::tuple;
using std::make_tuple;
using std::get;
using std::pair;
using std::make_pair;

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include <algorithm>
//#include "GL/gl.h"
//#include "../include/glad4.6/include/glad/glad.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"

#include "Geometry.h"
#include "model.h"
//#include "particle_generator.h"

#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif

//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l), camera(glm::vec3(0.0f, 20.0f, 100.0f))
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->cameraBrowser->value() == 1)
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD: {
			int k = Fl::event_key();
			int ks = Fl::event_state();
			switch (k)
			{
			case 'p': {
				// Print out the selected control point information
				if (selectedCube >= 0)
					printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
						selectedCube,
						m_pTrack->points[selectedCube].pos.x,
						m_pTrack->points[selectedCube].pos.y,
						m_pTrack->points[selectedCube].pos.z,
						m_pTrack->points[selectedCube].orient.x,
						m_pTrack->points[selectedCube].orient.y,
						m_pTrack->points[selectedCube].orient.z);
				else
					printf("Nothing Selected\n");

				return 1;
			}break;
			//case 'w':case 'W': {
			//	camera.ProcessKeyboard(FORWARD, deltaTime*0.05);
			//}break;
			//case 'a':case 'A': {
			//	camera.ProcessKeyboard(LEFT, deltaTime * 0.05);
			//}break;
			//case 's':case 'S': {
			//	camera.ProcessKeyboard(BACKWARD, deltaTime * 0.05);
			//}break;
			//case 'd':case 'D': {
			//	camera.ProcessKeyboard(RIGHT, deltaTime * 0.05);
			//}break;

			default:
				break;
			}
		}
		break;
		case FL_MOVE: {
			if (tw->cameraBrowser->value() != 4) break;
			static bool firstMouse = true;
			static float lastX, lastY;
			float xpos = Fl::event_x(), ypos = Fl::event_y();
			if (firstMouse)
			{
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			float xoffset = xpos - lastX;
			float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

			lastX = xpos;
			lastY = ypos;

			camera.ProcessMouseMovement(xoffset*0.7, yoffset*0.7);
		}
			break;
		case FL_Escape: {
			if (tw->cameraBrowser->value() == 4) {
				tw->cameraBrowser->select(1);
			}
		}break;
		case FL_MOUSEWHEEL: {
			if (Fl::event_dy() < 0) {
				camera.ProcessKeyboard(FORWARD, 15000);
			}
			else {
				camera.ProcessKeyboard(BACKWARD, 15000);
			}
		}break;
	}

	return Fl_Gl_Window::handle(event);
}

void dir_light() {
	float noAmbient[] = { 0.0f,0.0f,0.0f ,0.0f ,1.0f };
	float whiteDiffuse[] = { 1.0,1.0f ,1.0f ,1.0f };
	float position[] = { 0.0f,1.0f,0.0f ,0.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
}
void point_light() {
	float yellowAmbientDiffuse[] = { 0.0f,1.0f ,1.0f ,1.0f };
	float position[] = { 0.0f,40.0f ,0.0f ,1.0f };

	glLightfv(GL_LIGHT1, GL_AMBIENT, yellowAmbientDiffuse);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowAmbientDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);
}

unsigned int loadCubemap(vector<const GLchar*> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

glm::mat4 billboard(glm::mat4 mv,glm::vec3 translate) {
	glm::mat4 result(1.0f);
	float d = sqrtf(mv[0][0] * mv[0][0] + mv[1][0] * mv[1][0] + mv[2][0] * mv[2][0]);
	result[0][0] = d;
	result[1][1] = d;
	result[2][2] = d;

	result[0][3] = translate.x;
	result[1][3] = translate.y;
	result[2][3] = translate.z;
	return result;
	//glm::mat4 result(1.0f);
	//float d = sqrtf(mv[0][0] * mv[0][0] + mv[0][1] * mv[0][1] + mv[0][2] * mv[0][2]);
	//result[0][0] = d;
	//result[1][1] = d;
	//result[2][2] = d;

	//result[3][0] = translate.x;
	//result[3][1] = translate.y;
	//result[3][2] = translate.z;
	return result;
}
typedef struct com {
	float real;
	float vir;
}com;
void RenderScene();
void draw_scene_model(Model* ourModel, bool motion_activated, Shader* FboShader, glm::mat4 m_trans, glm::mat4 trans) {

	glClearColor(0.2f, 0.5f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(glGetUniformLocation(FboShader->Program,"model"), 1, GL_FALSE, glm::value_ptr(model));
	ourModel->Draw(*FboShader);

	unsigned int transformLoc = glGetUniformLocation(FboShader->Program, "trans");

	if (motion_activated)
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(m_trans));
	else
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

}

void TrainView::set_fbo(int* framebuffer, unsigned int* textureColorbuffer) {

	glGenFramebuffers(1, (GLuint*)framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	// create a color attachment texture
	glGenTextures(1, textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, *textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *textureColorbuffer, 0);



	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
void TrainView::esfera(int radio,bool draw_line) {
	float px, py, pz;
	int i, j;
	float incO = 2 * M_PI / sphere_divide;
	float incA = M_PI / sphere_divide;
	glBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i <= sphere_divide; i++) {
		for (j = 0; j <= sphere_divide; j++) {
			pz = cos(M_PI - (incA * j)) * radio;
			py = sin(M_PI - (incA * j)) * sin(incO * i) * radio;
			px = sin(M_PI - (incA * j)) * cos(incO * i) * radio;
			//printf("%lf%lf%lf\n", px, py, pz);
			glVertex3f(px, py, pz);
			pz = cos(M_PI - (incA * j)) * radio;
			py = sin(M_PI - (incA * j)) * sin(incO * (i + 1)) * radio;
			px = sin(M_PI - (incA * j)) * cos(incO * (i + 1)) * radio;
			glVertex3f(px, py, pz);
		}
	}
	glEnd();

	if (draw_line) {
		glColor3ub(0, 0, 0);
		glBegin(GL_LINE_STRIP);
		for (i = 0; i <= sphere_divide; i++) {
			for (j = 0; j <= sphere_divide; j++) {

				pz = cos(M_PI - (incA * j)) * radio;
				py = sin(M_PI - (incA * j)) * sin(incO * i) * radio;
				px = sin(M_PI - (incA * j)) * cos(incO * i) * radio;
				//printf("%lf%lf%lf\n", px, py, pz);
				glVertex3f(px, py, pz);
				pz = cos(M_PI - (incA * j)) * radio;
				py = sin(M_PI - (incA * j)) * sin(incO * (i + 1)) * radio;
				px = sin(M_PI - (incA * j)) * cos(incO * (i + 1)) * radio;
				glVertex3f(px, py, pz);

			}
		}
		glEnd();
	}
}

void makeCylinder(double height, double base) {
	int randd = rand() % 50 + 20;
	GLUquadric* obj = gluNewQuadric();
	//gluQuadricDrawStyle(obj, GLU_LINE);
	glColor3f(0.64f, 0.16f, 0.16f);
	glPushMatrix();
	glRotatef(-90.0f, 1.00f, 0.0f, 0.0f);
	gluCylinder(obj, base, base - (0.2f * base), height, 20.0f, 20.0f);
	glPopMatrix();
	gluDeleteQuadric(obj);
}


void  TrainView::makeTree(double height, double base) {
	double angle;
	makeCylinder(height, base);
	glTranslatef(0.0f, height, 0.0f);
	height -= height * 0.2f;
	base -= base * 0.3f;

	int ramas = rand() % 3 + 3;
	for (int a = 0;a < ramas;a++) {
		angle = rand() % 50 + 20;
		if (angle > 48)
			angle = -(rand() % 50 + 20);
		if (height > 1) {
			glPushMatrix();
			int randy = rand() % 4;
			if (randy % 2 == 0) {
				glRotatef(angle, 1, randy, 1);
			}
			else {
				glRotatef(angle, 1, (-randy), 1);
			}
			makeTree(height, base);
			glPopMatrix();
		}
		else {
			glColor3f(0.0f, 1.0f / (rand() % 3 + 1), 0.0f);
			esfera(1.0f, false);
		}
	}

	//gluSphere(0.2f, 10, 10);

}

void TrainView::reCreateTree() {

	// clear the draw buffer .
	glClear(GL_COLOR_BUFFER_BIT);   // Erase everything

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	makeaTree = glGenLists(1);
	glNewList(makeaTree, GL_COMPILE);
	makeTree(heightObj, baseObj);
	glEndList();
	
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{
	deltaTime = clock() - lastFrame;
	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
		if (!for_model) {
			for_model = new 
				Shader(
					"./Codes/shaders/model_loading.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/model_loading.frag");
		}

		if (!for_model_texture) {
			for_model_texture = new
				Shader(
					"./Codes/shaders/model_texture.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/model_texture.frag");
		}

		if (!tunnel) {
			tunnel = new Model("models/tunnel/tunnel.obj");
		}

		if (tunnel_tex == -1) {
			tunnel_tex = TextureFromFile("models/tunnel/tunnel.jpg", ".");
		}

		if (!steve) {
			steve = new Model("models/Steve/Steve.obj");
			//steve = new Model("models/monu9/monu9.obj");
		}

		if (!terrain) {
			//terrain = new Model("models/terrain/Mountain.obj");
			terrain = new Model("models/monu9/monu9.obj");
		}

		if (!rocket) {
			rocket = new Model("models/rocket/cartoon_rocket.obj");
		}

		if (!house) {
			house = new Model("models/house/Farmhouse OBJ.obj");
		}

		if (sun_tex == -1) {
			sun_tex = TextureFromFile("image/sun_baby.png", ".");
		}
		if (!sun_shader) {
			sun_shader = new
				Shader(
					"./Codes/shaders/sun.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/sun.frag");
		}

		if (terrain_tex == -1) {
			terrain_tex = TextureFromFile("models/terrain/BaseColor.png", ".");
		}

		if (!particle_shader) {
			particle_shader = new
				Shader(
					"./Codes/shaders/particle.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/particle.frag");
		}

		if (!particle_texture) {
			particle_texture = new Texture;
			particle_texture->id = TextureFromFile("image/particle.png", ".");
		}

		if (!Particles) {
			//Particles = new ParticleGenerator(particle_shader, particle_texture,500);
		}

				if (!this->screen) {
			this->screen = new
				Shader(
					"./Codes/shaders/screen.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/screen.frag");

			float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
			  // positions   // texCoords
			  -1.0f,  1.0f,  0.0f, 1.0f,
			  -1.0f, -1.0f,  0.0f, 0.0f,
			   1.0f, -1.0f,  1.0f, 0.0f,

			  -1.0f,  1.0f,  0.0f, 1.0f,
			   1.0f, -1.0f,  1.0f, 0.0f,
			   1.0f,  1.0f,  1.0f, 1.0f
			};
			// screen quad VAO
			glGenVertexArrays(1, &screen_quadVAO);
			glGenBuffers(1, &screen_quadVBO);
			glBindVertexArray(screen_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, screen_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			screen->Use();
			glUniform1i(glGetUniformLocation(screen->Program, "screenTexture"),0);

			glGenFramebuffers(1, &screen_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

			glGenTextures(1, &screen_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

			glGenRenderbuffers(1, &screen_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		if (!this->skybox) {
			this->skybox = new
				Shader(
					"./Codes/shaders/skybox.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/skybox.frag");
			float skyboxVertices[] = {
				// positions          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};
			// skybox VAO
			glGenVertexArrays(1, &skyboxVAO);
			glGenBuffers(1, &skyboxVBO);
			glBindVertexArray(skyboxVAO);
			glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			vector<const GLchar*> faces = {
			"image/skybox/right.jpg",
			"image/skybox/left.jpg",
			"image/skybox/top.jpg",
			"image/skybox/bottom.jpg",
			"image/skybox/front.jpg",
			"image/skybox/back.jpg",
			};
			cubemapTexture = loadCubemap(faces);
		}

		if (framebuffer[0] == -1) {
			for (int i = 0;i < 8;i++) {
				set_fbo(&framebuffer[i], &textureColorbuffer[i]);
				glUniform1i(glGetUniformLocation(screen->Program, ("TextureFBO" + to_string(i+1)).c_str()), i);
				trans[i] = glm::mat4(1.0f);
			}
		}
		if (!fbo_shader) {
			fbo_shader = new
				Shader(
					"./Codes/shaders/fbo.vert",
					nullptr, nullptr, nullptr,
					"./Codes/shaders/fbo.frag");
			//fbo_shader->Use();
			//glUniform1i(glGetUniformLocation(fbo_shader->Program, "texture_diffuse1"), 0);
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

		static int pre_w = w(), pre_h = h();
		// Set up the view port
		glViewport(0, 0, w(), h());
		if (pre_w != w() || pre_h != h()) {
			glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

			glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			/*
			////=======================================================
			// framebuffer configuration
			glBindFramebuffer(GL_FRAMEBUFFER, interactive_framebuffer);
			glBindTexture(GL_TEXTURE_2D, interactive_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, interactive_textureColorbuffer, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, interactive_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, interactive_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			*/
		}
		pre_w = w();
		pre_h = h();

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->cameraBrowser->value() == 3) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	if (tw->dir_L->value()) {
		dir_light();
	}
	else {
		glDisable(GL_LIGHT0);
	}
	if (tw->point_L->value()) {
		point_light();
	}
	else {
		glDisable(GL_LIGHT1);
	}
	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	//GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
	//GLfloat lightPosition2[]	= {1, 0, 0, 0};
	//GLfloat lightPosition3[]	= {0, -1, 0, 0};
	//GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	//GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	//GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	//GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

	//glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	//glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	//glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	//glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	//glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	GLfloat projection[16];
	GLfloat view[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, view);
	glm::mat4 view_without_translate = glm::mat4(glm::mat3(glm::make_mat4(view)));
	glm::mat4 view_inv = glm::inverse(glm::make_mat4(view));
	glm::vec3 my_pos(view_inv[3][0], view_inv[3][1], view_inv[3][2]);
	glm::mat4 model = glm::mat4(1.0f);

	current_trans = glm::make_mat4(projection) * glm::make_mat4(view);

	if (frame < 8) {
		if (frame % 8 != 0) {
			trans[frame % 8] = trans[frame % 8 - 1];
		}
		else {
			trans[0] = current_trans;
		}
	}
	else {

		trans[7] = trans[6];
		trans[6] = trans[5];
		trans[5] = trans[4];
		trans[4] = trans[3];
		trans[3] = trans[2];
		trans[2] = trans[1];
		trans[1] = trans[0];
		trans[0] = current_trans;
	}
	frame++;
	for (int i = 0;i < 8;i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer[i]);
		draw_scene_model(rocket, true, fbo_shader, trans[i], current_trans);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);
	// make sure we clear the framebuffer's content
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_FOG);
	float fogColor[4] = { 0.8, 0.8, 0.8, 1 };
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, 0.8);
	glHint(GL_FOG_HINT, GL_NICEST);
	glFogf(GL_FOG_START, 0.1);
	glFogf(GL_FOG_END, 200);

	// Fog
	const float fogDensity = 0.0045f;
	const glm::vec3 fogColor1 = glm::vec3(0.604f, 0.655f, 0.718f);
	const glm::vec3 fogColor2 = glm::vec3(0.631f, 0.651f, 0.698f);

	tunnel_pos = Pnt3f(70,0,0);



	
	model = glm::translate(model, glm::vec3(tunnel_pos.x, tunnel_pos.y, tunnel_pos.z));
	//model = glm::rotate(model,glm::radians(40.0f), glm::vec3(0,1,0));
	model = glm::scale(model, glm::vec3(50, 50, 25));

	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(glGetUniformLocation(for_model_texture->Program, "viewPos"), my_pos[0], my_pos[1], my_pos[2]);
	glUniform1f(glGetUniformLocation(for_model_texture->Program, "fog.Density"), fogDensity);
	glUniform3f(glGetUniformLocation(for_model_texture->Program, "fog.Color"), fogColor1[0], fogColor1[1], fogColor1[2]);
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, tunnel_tex);
	tunnel->Draw(*for_model_texture);

	glActiveTexture(GL_TEXTURE0);


	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0,0,0));
	model = glm::scale(model, glm::vec3(15, 15, 15));
	for_model_texture->Use();
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model_texture->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(for_model_texture->Program, "texture1"), 0);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	terrain->Draw(*for_model_texture);
	glActiveTexture(GL_TEXTURE0);
	


	

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-25, 0, 0));
	model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
	for_model->Use();
	glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(glGetUniformLocation(for_model->Program, "viewPos"), my_pos[0], my_pos[1], my_pos[2]);
	glUniform1f(glGetUniformLocation(for_model->Program, "fog.Density"), fogDensity);
	glUniform3f(glGetUniformLocation(for_model->Program, "fog.Color"), fogColor1[0], fogColor1[1], fogColor1[2]);
	rocket->Draw(*for_model);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-25, 0, 30));
	model = glm::scale(model, glm::vec3(1,1,1));
	glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	house->Draw(*for_model);


	sun_shader->Use();
	glUniformMatrix4fv(glGetUniformLocation(sun_shader->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(sun_shader->Program, "view"), 1, GL_FALSE, glm::value_ptr(billboard(glm::make_mat4(projection)*glm::make_mat4(view)*model,glm::vec3(0,80,0))));
	//glm::vec3 tmp = glm::normalize(glm::vec3(0.43f, 0.76f, -0.33f)) * 250.0f;
	//glUniform3f(glGetUniformLocation(sun_shader->Program, "sunPos"), tmp.x,tmp.y,tmp.z);
	glActiveTexture(GL_TEXTURE0); // active proper texture unit before binding
	glBindTexture(GL_TEXTURE_2D, sun_tex);
	Geometry::DrawPlane();


	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, 0, 0));
	model = glm::scale(model, glm::vec3(15, 15, 15));
	glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	//Particles->Draw();



	glDepthFunc(GL_LEQUAL);
	skybox->Use();
	glUniform1f(glGetUniformLocation(skybox->Program, "skybox"), cubemapTexture);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "projection"), 1, GL_FALSE, projection);
	glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "model_view"), 1, GL_FALSE, &view_without_translate[0][0]);
	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default

	//draw_tunnel();
	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************



	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	//drawFloor(200,10);
	//RenderScene();

	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	float sphere_distance = sqrtf(my_pos[0] * my_pos[0] + (my_pos[1]-40) * (my_pos[1] - 40) + my_pos[2] * my_pos[2]);
	if (sphere_distance < 0.01) sphere_distance = 0.01;
	sphere_divide = 1 / sphere_distance * 1000;
	if (sphere_divide > 100)sphere_divide = 100;
	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (tw->cameraBrowser->value() != 3) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
	//delete[] backpack_sh;
	//delete[] backpack;

	ProcessParticles();

	DrawParticles();

	// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	// clear all relevant buffers
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
	glClear(GL_COLOR_BUFFER_BIT);
	this->screen->Use();
	glUniform1i(glGetUniformLocation(screen->Program, "frame_buffer_type"), tw->frame_buffer_type->value());
	glUniform1f(glGetUniformLocation(screen->Program, "screen_w"), w());
	glUniform1f(glGetUniformLocation(screen->Program, "screen_h"), h());
	glUniform1f(glGetUniformLocation(screen->Program, "t"), tw->time * 20);
	glUniform2f(glGetUniformLocation(screen->Program, "u_texelStep"), 1.0f / w(), 1.0f / h());
	glUniform1i(glGetUniformLocation(screen->Program, "u_showEdges"), 0);
	glUniform1i(glGetUniformLocation(screen->Program, "u_fxaaOn"), 1);
	glUniform1i(glGetUniformLocation(screen->Program, "u_lumaThreshold"), 0.5);
	glUniform1i(glGetUniformLocation(screen->Program, "u_mulReduce"), 8.0f);
	glUniform1i(glGetUniformLocation(screen->Program, "u_minReduce"), 128.0f);
	glUniform1i(glGetUniformLocation(screen->Program, "u_maxSpan"),8.0f);
	glBindVertexArray(screen_quadVAO);
	glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);	// use the color attachment texture as the texture of the quad plane

	if (tw->frame_buffer_type->value() == 8) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[3]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[4]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[5]);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[6]);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer[7]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUseProgram(0);

}

void TrainView::draw_tunnel() {

}


//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());
	int cp_size = m_pTrack->points.size();
	// Check whether we use the world camp
	if (tw->cameraBrowser->value() == 1)
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->cameraBrowser->value() == 3) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else if(tw->cameraBrowser->value() == 2){
		//backpack_sh.use();
		GLfloat projection[16];
		GLfloat model_view[16];
		glClear(GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, aspect, 0.01f, 1000.0f);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		//backpack_sh.setMat4("projection", glm::make_mat4(projection));
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		int i;
		float t;
		if (tw->arcLength->value()) {
			i = train_i;
			t = train_t;
		}
		else {
			t = m_pTrack->trainU;
			i = floor(t);
			t -= i;
		}
		ControlPoint& p0 = m_pTrack->points[(i - 1 + cp_size) % cp_size];
		ControlPoint& p1 = m_pTrack->points[i % cp_size];
		ControlPoint& p2 = m_pTrack->points[(i + 1) % cp_size];
		ControlPoint& p3 = m_pTrack->points[(i + 2) % cp_size];
		Pnt3f orient = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);
		Pnt3f train_pos = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
		Pnt3f train_next = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t + 1.0f / DIVIDE_LINE);
		Pnt3f forward = train_next - train_pos;
		forward.normalize();
		Pnt3f cross = forward * orient;
		Pnt3f up = -1 * forward * cross;//���i�� forward *  forward * orient
		up.normalize();
		Pnt3f my_pos = train_pos + up * 5.0f;
		Pnt3f next_pos = train_next + up * 5.0f;
		gluLookAt(my_pos.x, my_pos.y, my_pos.z,
			next_pos.x, next_pos.y, next_pos.z, orient.x, orient.y, orient.z);
		glGetFloatv(GL_MODELVIEW_MATRIX, model_view);
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
	else if (tw->cameraBrowser->value() == 4) {
		glClear(GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, aspect, 0.01f, 1000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glm::mat4 view = camera.GetViewMatrix();
		glLoadMatrixf(glm::value_ptr(view));
	}
}
void glvertex_vec(Pnt3f in) {
	glVertex3f(in.x,in.y,in.z);
}
void glnormal_vec(Pnt3f in) {
	glNormal3f(in.x, in.y, in.z);
}
Pnt3f TrainView::GMT(const Pnt3f& p0, const Pnt3f& p1, const Pnt3f& p2, const Pnt3f& p3, int matrix_type, float t) {
	glm::mat4x4 M;
	float tesion = tw->tension->value();
	if (tesion < 0.01) tesion = 0.01;
	switch (matrix_type)
	{
	case 1: {		
		M = { 0, 0, 0, 0,
			0, 0, -1, 1,
			0, 0, 1, 0,
			0, 0, 0, 0 
		};
	}break;
	case 2: {
		M = {
			-1,2,-1,0,
			2/ tesion - 1,1 - 3/ tesion,0,1/ tesion,
			1-2/ tesion,3/ tesion-2,1,0,
			1,-1,0,0
		};
		M *= tesion;
	}break;
	case 3: default: {
		M = {
			-1,3,-3,1,
			3,-6,0,4,
			-3,3,3,1,
			1,0,0,0
		};
		M /= 6.0f;
	}break;
	}
	M = glm::transpose(M);
	glm::mat4x4 G = {
		{p0.x, p0.y, p0.z, 1.0f},
		{p1.x, p1.y, p1.z, 1.0f},
		{p2.x, p2.y, p2.z, 1.0f},
		{p3.x, p3.y, p3.z, 1.0f} };
	glm::vec4 T = { t * t * t, t * t, t, 1.0f };
	glm::vec4 result = G * M * T;
	return Pnt3f(result[0],result[1],result[2]);
}
//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{


	glPushMatrix();
	glShadeModel(GL_FLAT);
	glTranslatef(0, 40, 0);
	glRotatef(90.0f, 1.0, 0.0, 0.0);
	glColor4f(0.0, 0.0, 0.75, 0.5);
	glLineWidth(2.0);
	if (!doingShadows)glColor3ub(255, 255, 0);
	esfera(5,true);
	glPopMatrix();
	glShadeModel(GL_SMOOTH);
	static bool first_draw_tree = false;
	if (!first_draw_tree) {
		reCreateTree();
		first_draw_tree = true;
	}
	glPushMatrix();
	glTranslatef(20, 0, 30);
	glScalef(5, 5, 5);
	glCallList(makeaTree);
	glPopMatrix();

	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (tw->cameraBrowser->value() !=2) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	if(!doingShadows) glColor3ub(115, 148, 214);
	draw_pillar(2.0f);
	draw_track(doingShadows);
	if (tw->cameraBrowser->value() != 2) {
		draw_train(doingShadows);
	}

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}



void draw_sleeper(Pnt3f front,Pnt3f back,Pnt3f cross_t,Pnt3f up,bool doingShadows) {
	Pnt3f sleeper_up0 = front - up;
	Pnt3f sleeper_up1 = back - up;
	Pnt3f forward_nor = (front - back);
	forward_nor.normalize();
	Pnt3f cross_nor = cross_t;
	cross_nor.normalize();
	Pnt3f up_nor = -1 * up;//�����D���ƻ�n��-1
	up_nor.normalize();
	if (!doingShadows) glColor3ub(25, 25, 25);
	glLineWidth(1);
	glBegin(GL_LINES);
	glvertex_vec(front + cross_t);
	glvertex_vec(back + cross_t);

	glvertex_vec(back + cross_t);
	glvertex_vec(back - cross_t);

	glvertex_vec(back - cross_t);
	glvertex_vec(front - cross_t);

	glvertex_vec(front - cross_t);
	glvertex_vec(front + cross_t);

	//------------------------------
	glvertex_vec(front + cross_t);
	glvertex_vec(sleeper_up0 + cross_t);

	glvertex_vec(front - cross_t);
	glvertex_vec(sleeper_up0 - cross_t);

	glvertex_vec(back + cross_t);
	glvertex_vec(sleeper_up1 + cross_t);

	glvertex_vec(back - cross_t);
	glvertex_vec(sleeper_up1 - cross_t);

	//------------------------------
	glvertex_vec(sleeper_up0 + cross_t);
	glvertex_vec(sleeper_up1 + cross_t);

	glvertex_vec(sleeper_up1 + cross_t);
	glvertex_vec(sleeper_up1 - cross_t);

	glvertex_vec(sleeper_up1 - cross_t);
	glvertex_vec(sleeper_up0 - cross_t);

	glvertex_vec(sleeper_up0 - cross_t);
	glvertex_vec(sleeper_up0 + cross_t);
	glEnd();
	if (!doingShadows) glColor3ub(94, 38, 18);
	glBegin(GL_QUADS);
	glnormal_vec(-1 * up_nor);
	glvertex_vec(front + cross_t);
	glvertex_vec(back + cross_t);
	glvertex_vec(back - cross_t);
	glvertex_vec(front - cross_t);

	glnormal_vec(up_nor);
	glvertex_vec(sleeper_up0 + cross_t);
	glvertex_vec(sleeper_up1 + cross_t);
	glvertex_vec(sleeper_up1 - cross_t);
	glvertex_vec(sleeper_up0 - cross_t);

	glnormal_vec(-1 * cross_nor);
	glvertex_vec(sleeper_up0 - cross_t);
	glvertex_vec(front - cross_t);
	glvertex_vec(back - cross_t);
	glvertex_vec(sleeper_up1 - cross_t);

	glnormal_vec(cross_nor);
	glvertex_vec(sleeper_up0 + cross_t);
	glvertex_vec(front + cross_t);
	glvertex_vec(back + cross_t);
	glvertex_vec(sleeper_up1 + cross_t);

	glnormal_vec(forward_nor);
	glvertex_vec(front - cross_t);
	glvertex_vec(front + cross_t);
	glvertex_vec(sleeper_up0 + cross_t);
	glvertex_vec(sleeper_up0 - cross_t);

	glnormal_vec(-1* forward_nor);
	glvertex_vec(back - cross_t);
	glvertex_vec(back + cross_t);
	glvertex_vec(sleeper_up1 + cross_t);
	glvertex_vec(sleeper_up1 - cross_t);
	glEnd();
}
void TrainView::draw_first_sleeper(bool doingShadows) {
	float percent = 1.0f / DIVIDE_LINE;
	Pnt3f prepreQ = GMT(m_pTrack->points[m_pTrack->points.size() - 2].pos, m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, tw->splineBrowser->value(), 1.0f - percent);
	Pnt3f preQ = GMT(m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, m_pTrack->points[2].pos, tw->splineBrowser->value(), 0.0f);
	Pnt3f firstO = GMT(m_pTrack->points.back().orient, m_pTrack->points.front().orient, m_pTrack->points[1].orient, m_pTrack->points[2].orient, tw->splineBrowser->value(), 0.0f);
	firstO.normalize();
	Pnt3f firstcross = (preQ - prepreQ) * firstO;
	firstcross.normalize();
	firstcross = firstcross * sleeper_width;
	Pnt3f firstbackward = (preQ - prepreQ);
	firstbackward.normalize();
	Pnt3f firstup = firstbackward * firstcross;
	firstup.normalize();
	Pnt3f first_pre_sleeper = preQ - firstbackward * sleeper_length;
	draw_sleeper(preQ, first_pre_sleeper, firstcross, firstup, doingShadows);
}

void TrainView::draw_track(bool doingShadows) {
	int cp_size = m_pTrack->points.size();
	arc_length = 0.0f;
	float T = 0.0f;
	float percent = 1.0f / DIVIDE_LINE;
	Pnt3f prepreQ = GMT(m_pTrack->points[cp_size - 2].pos, m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, tw->splineBrowser->value(), 1.0f - percent);
	Pnt3f preQ = GMT(m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, m_pTrack->points[2].pos, tw->splineBrowser->value(), 0.0f);
	Pnt3f preO = GMT(m_pTrack->points.back().orient, m_pTrack->points.front().orient, m_pTrack->points[1].orient, m_pTrack->points[2].orient, tw->splineBrowser->value(), 0.0f);
	Pnt3f pre_cross = (preQ - prepreQ) * preO.normalize();
	pre_cross = pre_cross.normalize() * rail_width;
	draw_first_sleeper(doingShadows);//�קK�Ĥ@��ŻؤӤj
	bool sleeper = false;
	bool get_train_pos = false;
	for (int i = 0;i < cp_size;i++) {
		ControlPoint& p0 = m_pTrack->points[(i - 1 + cp_size) % cp_size];
		ControlPoint& p1 = m_pTrack->points[i % cp_size];
		ControlPoint& p2 = m_pTrack->points[(i + 1) % cp_size];
		ControlPoint& p3 = m_pTrack->points[(i + 2) % cp_size];
		float t = percent;
		for (int j = 1;j < DIVIDE_LINE;j++) {
			Pnt3f Q = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
			Pnt3f O = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);
			Pnt3f backward = (Q - preQ);
			Pnt3f cross_t = backward * O.normalize();
			Pnt3f up = backward * cross_t.normalize();
			//up = up * -1;
			up.normalize();
			cross_t = cross_t * rail_width;
			glLineWidth(5);
			if (!doingShadows)glColor3ub(192, 192, 192);
			glBegin(GL_LINES);
			glvertex_vec(preQ + cross_t);
			glvertex_vec(Q + cross_t);
			glvertex_vec(preQ - cross_t);
			glvertex_vec(Q - cross_t);
			glvertex_vec(prepreQ - pre_cross);//�ɱ��_��
			glvertex_vec(preQ - cross_t);//�ɱ��_��
			glvertex_vec(prepreQ + pre_cross);//�ɱ��_��
			glvertex_vec(preQ + cross_t);//�ɱ��_��
			glEnd();
			pre_cross = cross_t;
			arc_length += backward.length();
			if (!get_train_pos && arc_length > m_pTrack->trainU) {
				get_train_pos = true;
				train_i = i;
				train_t = t;
				physics_add = (preQ.y - Q.y) * 20.0f;
			}
			T += backward.length();
			if (sleeper && T >= sleeper_length) {
				Pnt3f pre_sleeper = Q - backward.normalize() * sleeper_length;
				draw_sleeper(Q, pre_sleeper, cross_t.normalize() * sleeper_width, up, doingShadows);
				T -= sleeper_length;
				sleeper = !sleeper;
			}
			else if (!sleeper && T >= space_length) {
				T -= space_length;
				sleeper = !sleeper;
			}
			prepreQ = preQ;
			preQ = Q;
			t += percent;
		}
	}
	//std::cout << arc_length << std::endl;
}

void TrainView::draw_train(bool doingShadows) {
	int i;
	float t;
	if (tw->arcLength->value()) {
		i = train_i;
		t = train_t;
	}
	else {
		t = m_pTrack->trainU;
		i = floor(t);
		t -= i;
	}
	int cp_size = m_pTrack->points.size();
	float percent = 1.0f / DIVIDE_LINE;
	ControlPoint p0 = m_pTrack->points[(i - 1 + cp_size) % cp_size];
	ControlPoint p1 = m_pTrack->points[i % cp_size];
	ControlPoint p2 = m_pTrack->points[(i + 1) % cp_size];
	ControlPoint p3 = m_pTrack->points[(i + 2) % cp_size];
	for (int j = 0;j < car_amount;j++) {
		Pnt3f train_pos = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
		Pnt3f train_next = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t + percent);
		Pnt3f train_ori = GMT(p0.orient, p1.orient, p2.orient, p3.orient, tw->splineBrowser->value(), t);
		Pnt3f forward = train_next - train_pos;
		forward.normalize();//�[�o�Ӥ~���|�ܼu����
		Pnt3f cross_t = forward * train_ori;
		cross_t.normalize();
		cross_t = cross_t * 10.0f;
		Pnt3f up = -1 * forward * cross_t;
		forward = forward * 0.5;
		cross_t = cross_t * 0.5;
		up = up * 0.5;
		Pnt3f steve_pos = train_pos + up * 3.0f;


		Pnt3f train_down_front = train_pos + forward * 10.0f + up * 1.0f;
		Pnt3f train_down_back = train_pos - forward * 10.0f + up * 1.0f;
		Pnt3f train_up_front = train_down_front + up * 2.0f;
		Pnt3f train_up_back = train_down_back + up * 2.0f;
		float car_length = (train_down_front - train_down_back).length();
		Pnt3f forward_nor = (train_next - train_pos);
		forward_nor.normalize();
		Pnt3f cross_nor = cross_t;
		cross_nor.normalize();
		Pnt3f up_nor = up;
		up_nor.normalize();
		

		static float pret = clock();
		pair<glm::vec3, glm::vec3> tmp = make_pair(glm::vec3(steve_pos.x, steve_pos.y, steve_pos.z), glm::vec3(3, 3, 3));
		//Particles->Update((clock() - pret) / 400, tmp, 2, glm::vec3(1, 1, 1));

		pret = clock();

		if (j == 0 && !doingShadows) {
			

			float rotation[16] = {
				forward_nor.x, forward_nor.y, forward_nor.z, 0.0,
				cross_nor.x, cross_nor.y, cross_nor.z, 0.0,
				up_nor.x, up_nor.y, up_nor.z, 0.0,
				0.0, 0.0, 0.0, 1.0
			};

			GLfloat projection[16];
			GLfloat view_ptr[16];
			glGetFloatv(GL_PROJECTION_MATRIX, projection);
			glGetFloatv(GL_MODELVIEW_MATRIX, view_ptr);
			glm::mat4 view = glm::make_mat4(view_ptr);
			view = glm::translate(view, glm::vec3(steve_pos.x, steve_pos.y, steve_pos.z));
			view = view * glm::make_mat4(rotation);
			view = glm::rotate(view,glm::radians(90.0f),glm::vec3(1,0,0));//why???

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(1, 1, 1));


			for_model->Use();
			glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "projection"), 1, GL_FALSE, projection);
			glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(for_model->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			steve->Draw(*for_model);

			particle_shader->Use();
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(15, 15, 15));
			glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "projection"), 1, GL_FALSE, projection);
			glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(particle_shader->Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			//Particles->Draw();
			
			glUseProgram(0);
		}


		


		if (!doingShadows)glColor3ub(23, 122, 222);
		glBegin(GL_QUADS);//�U
		glnormal_vec(-1 * up_nor);
		glvertex_vec(train_down_front + cross_t);
		glvertex_vec(train_down_back + cross_t);
		glvertex_vec(train_down_back - cross_t);
		glvertex_vec(train_down_front - cross_t);
		glEnd();
		glBegin(GL_QUADS);//�W
		glnormal_vec(up_nor);
		glvertex_vec(train_up_front + cross_t);
		glvertex_vec(train_up_back + cross_t);
		glvertex_vec(train_up_back - cross_t);
		glvertex_vec(train_up_front - cross_t);
		glEnd();
		glBegin(GL_QUADS);//��
		glnormal_vec(-1 * cross_nor);
		glvertex_vec(train_down_front - cross_t);
		glvertex_vec(train_up_front - cross_t);
		glvertex_vec(train_up_back - cross_t);
		glvertex_vec(train_down_back - cross_t);
		glEnd();
		glBegin(GL_QUADS);//�k
		glnormal_vec(cross_nor);
		glvertex_vec(train_down_front + cross_t);
		glvertex_vec(train_up_front + cross_t);
		glvertex_vec(train_up_back + cross_t);
		glvertex_vec(train_down_back + cross_t);
		glEnd();
		glBegin(GL_QUADS);//�e
		glnormal_vec(forward_nor);
		glvertex_vec(train_down_front - cross_t);
		glvertex_vec(train_down_front + cross_t);
		glvertex_vec(train_up_front + cross_t);
		glvertex_vec(train_up_front - cross_t);
		glEnd();
		glBegin(GL_QUADS);//��
		glnormal_vec(-1 * forward_nor);
		glvertex_vec(train_down_back - cross_t);
		glvertex_vec(train_down_back + cross_t);
		glvertex_vec(train_up_back + cross_t);
		glvertex_vec(train_up_back - cross_t);
		glEnd();
		draw_wheel(train_pos - cross_nor * sleeper_width + up_nor * 5.0f, forward_nor, cross_nor, up_nor, 0.9f, 0.1f, color_pos + j * 3, doingShadows);
		draw_wheel(train_pos + cross_nor * sleeper_width + up_nor * 5.0f, forward_nor, cross_nor, up_nor, 0.9f, 0.1f, color_pos + j * 3, doingShadows);
		float collect_len = 0.0f;
		for (;;t-=percent) {
			train_pos = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
			train_next = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t + percent);
			collect_len += (train_next - train_pos).length();
			if (collect_len >= car_length + car_to_car_distance) break;
			if (t < 0.0f) {
				t = 1.0f;
				--i;
				if (i < 0) i = cp_size - 1;
				p0 = m_pTrack->points[(i - 1 + cp_size) % cp_size];
				p1 = m_pTrack->points[i % cp_size];
				p2 = m_pTrack->points[(i + 1) % cp_size];
				p3 = m_pTrack->points[(i + 2) % cp_size];
			}
		}
	}




}

void TrainView::no_arc_to_arc() {
	int cp_size = m_pTrack->points.size();
	float percent = 1.0f / DIVIDE_LINE;
	train_i = floor(m_pTrack->trainU);
	train_t = m_pTrack->trainU - train_i;
	m_pTrack->trainU = 0.0f;
	Pnt3f preQ = GMT(m_pTrack->points.back().pos, m_pTrack->points.front().pos, m_pTrack->points[1].pos, m_pTrack->points[2].pos, tw->splineBrowser->value(), 0.0f);
	for (int i = 0;i <= train_i;i++) {
		ControlPoint& p0 = m_pTrack->points[(i - 1 + cp_size) % cp_size];
		ControlPoint& p1 = m_pTrack->points[i % cp_size];
		ControlPoint& p2 = m_pTrack->points[(i + 1) % cp_size];
		ControlPoint& p3 = m_pTrack->points[(i + 2) % cp_size];
		float t = percent;
		float t_DIVIDE_LINE = i != train_i ? DIVIDE_LINE : train_t * DIVIDE_LINE;
		for (int j = 1;j < t_DIVIDE_LINE;j++) {
			Pnt3f Q = GMT(p0.pos, p1.pos, p2.pos, p3.pos, tw->splineBrowser->value(), t);
			Pnt3f backward = (Q - preQ);
			m_pTrack->trainU += backward.length();
			preQ = Q;
			t += percent;
		}
	}
}
void TrainView::draw_wheel(Pnt3f center,Pnt3f forward,Pnt3f cross,Pnt3f up,float r,float thickness,int color_pos,bool doingShadows) {
	int divide = 19;
	glm::vec2 circle[19] = { glm::vec2(1.000, 0.000),glm::vec2(0.940, 0.342),glm::vec2(0.766, 0.643),glm::vec2(0.500, 0.866),
		glm::vec2(0.174, 0.985),glm::vec2(-0.173, 0.985),glm::vec2(-0.499, 0.867),glm::vec2(-0.765, 0.644),glm::vec2(-0.939, 0.343),
		glm::vec2(-1.000, 0.002),glm::vec2(-0.940, -0.340),glm::vec2(-0.767, -0.641),glm::vec2(-0.502, -0.865),glm::vec2(-0.176, -0.984),
		glm::vec2(0.171, -0.985),glm::vec2(0.498, -0.867),glm::vec2(0.764, -0.645),glm::vec2(0.939, -0.345),glm::vec2(1.000, -0.003) };
	int color[19][3] = {
		{98,95,100},{89,75,100},{81,55,100},{75,35,100},{64,15,100},{55,0,96},{49,0,84},{43,0,74},{37,0,64},{31,0,54},
		{0,0,50},{0,0,60},{0,0,70},{0,0,80},{0,0,90},{3,3,100},{23,23,100},{43,43,100},{64,64,100}
	};
	float rotation[16] = {
		forward.x, forward.y, forward.z, 0.0,
		cross.x, cross.y, cross.z, 0.0,
		up.x, up.y, up.z, 0.0,
		0.0, 0.0, 0.0, 1.0
	};
	for (int i = 0; i < 19; ++i) circle[i] *= r;
	glPushMatrix();
	glTranslatef(center.x, center.y, center.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f,5.0f);
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < 19; ++i) {
		if (!doingShadows) glColor3b(color[(color_pos + i) % divide][0], color[(color_pos + i) % divide][1], color[(color_pos + i) % divide][2]);
		glVertex3f(circle[i][0], thickness, circle[i][1]);
		glVertex3f(circle[(i + 1)%divide][0], thickness, circle[(i + 1) % divide][1]);
		glVertex3f(0, thickness,0);
	}
	for (int i = 0; i < 19; ++i) {
		if (!doingShadows) glColor3b(color[(color_pos + i) % divide][0], color[(color_pos + i) % divide][1], color[(color_pos + i) % divide][2]);
		glVertex3f(circle[i][0], -thickness, circle[i][1]);
		glVertex3f(circle[(i + 1) % divide][0], -thickness, circle[(i + 1) % divide][1]);
		glVertex3f(0, -thickness, 0);

	}
	glEnd();
	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i < 19; ++i) {
		if (!doingShadows) glColor3b(color[(color_pos+i)%divide][0], color[(color_pos + i) % divide][1], color[(color_pos + i) % divide][2]);
		glVertex3f(circle[i][0], thickness, circle[i][1]);
		glVertex3f(circle[i][0], -thickness,circle[i][1]);
	}
	glEnd();
	glPopMatrix();
}
void TrainView::draw_pillar(float r){
	glm::vec2 circle[19] = { glm::vec2(1.000, 0.000),glm::vec2(0.940, 0.342),glm::vec2(0.766, 0.643),glm::vec2(0.500, 0.866),
		glm::vec2(0.174, 0.985),glm::vec2(-0.173, 0.985),glm::vec2(-0.499, 0.867),glm::vec2(-0.765, 0.644),glm::vec2(-0.939, 0.343),
		glm::vec2(-1.000, 0.002),glm::vec2(-0.940, -0.340),glm::vec2(-0.767, -0.641),glm::vec2(-0.502, -0.865),glm::vec2(-0.176, -0.984),
		glm::vec2(0.171, -0.985),glm::vec2(0.498, -0.867),glm::vec2(0.764, -0.645),glm::vec2(0.939, -0.345),glm::vec2(1.000, -0.003) };
	for (int i = 0; i < 19; ++i) circle[i] *= r;
	for (ControlPoint cp : m_pTrack->points){
		Pnt3f pos = cp.pos;
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i < 19; ++i){
			glVertex3f(pos.x + circle[i][0], pos.y, pos.z + circle[i][1]);
			glVertex3f(pos.x + circle[i][0], 0.0f, pos.z + circle[i][1]);
		}
		glEnd();
	}
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}




com cmul(com a, com b)
{
	com c;
	c.real = a.real * b.real - a.vir * b.vir;
	c.vir = a.real * b.vir + a.vir * b.real;
	return c;
}

com cadd(com a, com b)
{
	com c;
	c.real = a.real + b.real;
	c.vir = a.vir + b.vir;
	return c;
}

float m(com a)
{
	return sqrt(a.real * a.real + a.vir * a.vir);
}

float cfunc(com a, int iter)
{
	//const c: real part affects pattern complication, virtual part affects symmetry
	com c = { -1,0.3 };
	if (iter)
	{
		com d = cadd(cmul(a, a), c);
		com e = cadd(cmul(d, d), c);
		return cfunc(e, --iter);
	}
	else return (int)(m(a) / 4 * 10);
}

void RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT);
	float i, j;
	glBegin(GL_POINTS);
	float d;
	com a;

	//draw the fractal point
	for (i = -2;i <= 2;i += 0.01)
		for (j = -2;j <= 2;j += 0.01)
		{
			a.real = i;
			a.vir = j;
			d = cfunc(a, 6);
			d = d / 10;
			glColor3f(d, d * 1.2, d * 1.5);
			glVertex2f(i,j);
		}

	glEnd();
	glFlush();
}