//Stephen Blanchard
//swb4062 - Fall 2015
//CMPS 415 - Assignment 2


//#define USE_PRIMITIVE_RESTART
//#include <stdio.h> 
//#include <stdlib.h>
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstring>
#include <ostream>
#include <ostream>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <math.h>
#include <gmtl/gmtl.h>
#include <gmtl/Matrix.h>
#include <gmtl/VecOps.h>
#include "LoadShaders.h"
#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace gmtl;

Matrix44f cameraTrans, cameraRot;	//Camera transform matrices
bool menu = false;					//a bool to keep the menu from reprinting
Matrix44f palm, finger, tip;		//4x4 matrices for the 3 VAO objects

Matrix44f translationMatrix;		//matrix for translations
Matrix44f rotationMatrix;			//matrix for rotations
Matrix44f scaleMatrix;				//matrix for scaling
Matrix44f temp;						//temp matrix to store the product of matrix multiplication
char object = ' ', axis = ' ';		//denote which object, axis and frame is chosen
char frame = ' ';					//denotes which frame to rotate or translate about.
float x, y, z;						//variables for current x, y, z of object used to translate to and from origin for transforms.

const Vec3f offSetFinger = Vec3f(0.0f, 0.5f, 0.0f);
struct Keyframe
{
	unsigned long time; // Timestamp, milliseconds since first record. Assume nondecreasing order.
	float palm_p[3];    // palm position w.r.t. world (x, y, z)
	float palm_q[4];    // palm orientation w.r.t. world, quaternion (a, b, c, s) a.k.a. (x, y, z, w)
	float joint[16];    // finger joint angles (in radians). See list above.
	float ball_p[3];    // ball position
	float ball_q[4];    // ball orientation
};

//function to handle all translations situationally
void translate(float tx, float ty, float tz){
	//set the translation based on parameters
	translationMatrix.set
		(1.0f, 0.0f, 0.0f, tx,
		 0.0f, 1.0f, 0.0f, ty,
		 0.0f, 0.0f, 1.0f, tz,
		 0.0f, 0.0f, 0.0f, 1.0f
		);
	//if translating the cube
	if (object == 'p'){
		//translate locally (transform is on the right)
		if (frame == 'l')
			mult(temp, palm, translationMatrix);
		//or translate by world (transform is on the left)
		if (frame == 'w')
			mult(temp, translationMatrix, palm);

		palm = temp;
	}
	
	if (object == 'f'){
		if (frame == 'l')
			mult(temp, finger, translationMatrix);
		if (frame == 'w')
			mult(temp, translationMatrix, finger);
		finger = temp;
	}

	if (object == 't'){
		if (frame == 'l')
			mult(temp, tip, translationMatrix);
		if (frame == 'w')
			mult(temp, translationMatrix, tip);
		tip = temp;
	}
}

//function to handle scaling based on passed parameters
void scale(float sx, float sy, float sz){
	//full scaling is only required for the finger or rectangle
	scaleMatrix.set
		(sx, 0.0f, 0.0f, 0.0f,
		0.0f, sy, 0.0f, 0.0f,
		0.0f, 0.0f, sz, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
	if (object == 'f'){
		x = finger.mData[12];
		y = finger.mData[13];
		z = finger.mData[14];
		//translate to the origin
		frame = 'w';
		translate(-x, -y, -z);
		//scale
		mult(finger, scaleMatrix, temp);
		//translate back
		translate(x, y, z);
	}
	//The tip only scales on the x axis
	if (object == 't'){
		scaleMatrix.set
			(sx, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);

		x = tip.mData[12];
		y = tip.mData[13];
		z = tip.mData[14];

		frame = 'w';
		translate(-x, -y, -z);
		//scale
		mult(tip, temp, scaleMatrix);
		//translate back
		translate(x, y, z);
	}

}

//function to handle all rotations
void rotate(float theta){
	//The user choose an axis to rotate about and the rotations are handled according to this choice
	if (axis == 'x')
		rotationMatrix.set
			(1.0f, 0.0f, 0.0f, 0.0f,
			 0.0f, cos(theta), -sin(theta), 0.0f,
			 0.0f, sin(theta), cos(theta), 0.0f,
			 0.0f, 0.0f, 0.0f, 1.0f
			);

	if (axis == 'y')
		rotationMatrix.set
			(cos(theta), 0.0f, sin(theta), 0.0f,
			 0.0f, 1.0f, 0.0f, 0.0f,
			 -sin(theta), 0.0f, cos(theta), 0.0f,
			 0.0f, 0.0f, 0.0f, 1.0f
			);

	if (axis == 'z')
		rotationMatrix.set
		(cos(theta), -sin(theta), 0.0f, 0.0f,
		sin(theta), cos(theta), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
	
	//for the palm or cube, each axis is required as well as local and world rotations
	if (object == 'p'){

		x = palm.mData[12];
		y = palm.mData[13];
		z = palm.mData[14];
		
		//translate depending on chosen frame
		if (frame == 'w'){
			//translate to origin
			translate(-x, -y, -z);
			mult(palm, rotationMatrix, temp);
			//translate back
			translate(x, y, z);
		}
		if (frame == 'l'){
			frame = 'w';
			translate(-x, -y, -z);
			mult(palm, temp, rotationMatrix);
			translate(x, y, z);
			frame = 'l';
		}

	}
	//rotation for finger/rect
	if (object == 'f'){
		x = finger.mData[12];
		y = finger.mData[13];
		z = finger.mData[14];
		//translate to origin
		frame = 'w';
		translate(-x, -y, -z);
		//only rotations about the local frame are requried
		mult(finger, temp, rotationMatrix);
		//translate back
		translate(x, y, z);
	}
	//tip rotation
	if (object == 't'){
		x = tip.mData[12];
		y = tip.mData[13];
		z = tip.mData[14];
		//translate to origin
		frame = 'w';
		translate(-x, -y, -z);
		//rotate about chosen world axis world
		mult(tip, rotationMatrix, temp);
		//translate back
		translate(x, y, z);
	}

}
//function to simulate camera movements
void moveCamera(float theta){
	x = palm.mData[12];
	y = palm.mData[13];
	z = palm.mData[14];
	//set a camera translation matrix to translate the "camera" to the origin
	cameraTrans.set(1.0f, 0.0f, 0.0f, -x,
			   0.0f, 1.0f, 0.0f, -y,
			   0.0f, 0.0f, 1.0f, -z,
			   0.0f, 0.0f, 0.0f, 1.0f);
	//set up the camera rotation based on the axis chosen by pressing the arrow keys
	if (axis == 'x')
		cameraRot.set
		(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cos(theta), -sin(theta), 0.0f,
		0.0f, sin(theta), cos(theta), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	if (axis == 'y')
		cameraRot.set
		(cos(theta), 0.0f, sin(theta), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sin(theta), 0.0f, cos(theta), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
	//translate the camera, rotate all of the objects and translate back
	mult(temp, cameraTrans, palm);
	mult(palm, cameraRot, temp);
	cameraTrans.set(1.0f, 0.0f, 0.0f, x,
		0.0f, 1.0f, 0.0f, y,
		0.0f, 0.0f, 1.0f, z,
		0.0f, 0.0f, 0.0f, 1.0f);
	mult(temp, cameraTrans, palm);
	palm = temp;

	x = finger.mData[12];
	y = finger.mData[13];
	z = finger.mData[14];
	cameraTrans.set(1.0f, 0.0f, 0.0f, -x,
		0.0f, 1.0f, 0.0f, -y,
		0.0f, 0.0f, 1.0f, -z,
		0.0f, 0.0f, 0.0f, 1.0f);
	mult(temp, cameraTrans, finger);
	mult(finger, cameraRot, temp);
	cameraTrans.set(1.0f, 0.0f, 0.0f, x,
		0.0f, 1.0f, 0.0f, y,
		0.0f, 0.0f, 1.0f, z,
		0.0f, 0.0f, 0.0f, 1.0f);
	mult(temp, cameraTrans, finger);
	finger = temp;

	x = tip.mData[12];
	y = tip.mData[13];
	z = tip.mData[14];
	cameraTrans.set(1.0f, 0.0f, 0.0f, -x,
		0.0f, 1.0f, 0.0f, -y,
		0.0f, 0.0f, 1.0f, -z,
		0.0f, 0.0f, 0.0f, 1.0f);
	mult(temp, cameraTrans, tip);
	mult(tip, cameraRot, temp);
	cameraTrans.set(1.0f, 0.0f, 0.0f, x,
		0.0f, 1.0f, 0.0f, y,
		0.0f, 0.0f, 1.0f, z,
		0.0f, 0.0f, 0.0f, 1.0f);
	mult(temp, cameraTrans, tip);
	tip = temp;

}

//function to put everything back into it's original transform
void reinitialize(){

	gmtl::identity(palm);

	gmtl::identity(finger);
	object = 'f';
	frame = 'w';
	translate(offSetFinger.mData[0], offSetFinger.mData[1], offSetFinger.mData[2]);
	object = ' ';
	frame = ' ';

	gmtl::identity(tip);

}

// Vertex list (corners of a palm):
static const GLfloat palm_vertex_buffer_data[] = {
	-0.15f, -0.1f, -0.1f,	//40, 40
	-0.30f, -0.25f, 0.1f,	//25, 25
	-0.15f, 0.30f, -0.1f,	//40, 80
	-0.30f, 0.15f, 0.1f,	//25, 65
	0.25f, -0.1f, -0.1f,	//80, 40
	0.10f, -0.25f, 0.1f,	//65, 25
	0.25f, 0.30f, -0.1f,	//80, 80
	0.10f, 0.15f, 0.1f		//65, 65
};

// Vertex list (corners of a finger):
static const GLfloat finger_vertex_buffer_data[] = {
	-0.60f, 0.05f, -0.1f,
	-0.675f, -0.025f, 0.1f,
	-0.60f, 0.10f, -0.1f,
	-0.675f, 0.025f, 0.1f,
	0.70f, 0.05f, -0.1f,
	0.625f, -0.025f, 0.1f,
	0.70f, 0.10f, -0.1f,
	0.625f, 0.05f, 0.1f
};

// Vertex list (corners of a tip):
static const GLfloat tip_vertex_buffer_data[] = {
	-0.05f, -0.025f, -0.05f,
	-0.0875f, -0.0625f, 0.05f,
	-0.05f, 0.075f, -0.05f,
	-0.0875f, 0.0375f, 0.05f,
	0.05f, -0.025f, -0.05f,
	0.0125f, -0.0625f, 0.05f,
	0.05f, 0.075f, -0.05f,
	0.0125f, 0.0375f, 0.05f
};

// RGB for cube/palm.
static const GLfloat palm_color_buffer_data[] = {
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f,
	1.0f, 0.0f, 0.50f
};
// RGB for rectangle/finger
static const GLfloat finger_color_buffer_data[] = {
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f,
	1.0f, 0.50f, 0.0f
};

// RGB for tip
static const GLfloat tip_color_buffer_data[] = {
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};


// Index list. Using triangle strips as in OpenGL book example 3.7.
static const GLushort palm_indices_buffer_data[] =
{
	0, 1, 2, 3, 6, 7, 4, 5, // First strip
	0xFFFF, // restart primitive
	2, 6, 0, 4, 1, 5, 3, 7 // Second strip
};

//Index list for rect/finger
static const GLushort finger_indices_buffer_data[] =
{
	0, 1, 2, 3, 6, 7, 4, 5,// First strip
	0xFFFF, // restart primitive
	2, 6, 0, 4, 1, 5, 3, 7 // Second strip
};

//Index list for tip
static const GLushort tip_indices_buffer_data[] =
{
	0, 1, 2, 3, 6, 7, 4, 5, // First strip
	0xFFFF, // restart primitive
	2, 6, 0, 4, 1, 5, 3, 7 // Second strip
};


// Vertex array object
GLuint palmVertexArrayID[2];
GLuint fingerVertexArrayID[2];
GLuint tipVertexArrayID[2];

// Vertex buffer objects
GLuint vertexbufferpalm;
GLuint colorbufferpalm;
GLuint indexbufferpalm;
GLuint vertexbufferfinger;
GLuint indexbufferfinger;
GLuint colorbufferfinger;
GLuint vertexbuffertip;
GLuint colorbuffertip;
GLuint indexbuffertip;


// Parameter location for passing a matrix to vertex shader
GLuint Matrix_loc;
// Parameter locations for passing data to shaders
GLuint vertposition_loc, vertcolor_loc;

GLenum errCode;
const GLubyte *errString;

void init(){
	// Enable depth test (visible surface determination)
	glEnable(GL_DEPTH_TEST);

	// OpenGL background color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Load/compile/link shaders and set to use for rendering
	ShaderInfo shaders[] = { { GL_VERTEX_SHADER, "Cube_Vertex_Shader.vert" },
	{ GL_FRAGMENT_SHADER, "Cube_Fragment_Shader.frag" },
	{ GL_NONE, NULL } };

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);

	//Get the shader parameter locations for passing data to shaders
	vertposition_loc = glGetAttribLocation(program, "vertexPosition");
	vertcolor_loc = glGetAttribLocation(program, "vertexColor");
	Matrix_loc = glGetUniformLocation(program, "Matrix");

	// initialize three GMTL matrices as identity
	gmtl::identity(palm);
	object = 'p';
	frame = 'w';
	translate(-0.50f, 0.0f, 0.0f);
	//move the finger/rectangle object away from the other two as specified in instructions
	gmtl::identity(finger);
	object = 'f';
	scale(0.25f, 0.7f, 0.0f);
	translate(0.0f, 0.2f, 0.0f);
	//initialize tip
	gmtl::identity(tip);

	/*** VERTEX ARRAY OBJECT SETUP***/
	// Create/Generate the Vertex Array Object
	glGenVertexArrays(1, &palmVertexArrayID[0]);
	
	// Bind the Vertex Array Object
	glBindVertexArray(palmVertexArrayID[0]);

	// Create/Generate the Vertex Buffer Object for the vertices.
	glGenBuffers(1, &vertexbufferpalm);

	// Bind the Vertex Buffer Object.
	glBindBuffer(GL_ARRAY_BUFFER, vertexbufferpalm);

	// Transfer data in to graphics system
	glBufferData(GL_ARRAY_BUFFER, sizeof(palm_vertex_buffer_data), palm_vertex_buffer_data, GL_STATIC_DRAW);

	// Specify data location and organization
	glVertexAttribPointer(vertposition_loc, // This number must match the layout in the shader
		3, // Size
		GL_FLOAT, // Type
		GL_FALSE, // Is normalized
		0, ((void*)0));
	// Enable the use of this array
	glEnableVertexAttribArray(vertposition_loc);

	// Similarly, set up the color buffer.
	glGenBuffers(1, &colorbufferpalm);

	glBindBuffer(GL_ARRAY_BUFFER, colorbufferpalm);

	glBufferData(GL_ARRAY_BUFFER, sizeof(palm_color_buffer_data), palm_color_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(vertcolor_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertcolor_loc);

	// Set up the element (index) array buffer and copy in data
	glGenBuffers(1, &indexbufferpalm);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbufferpalm);

	// Transfer data
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(palm_indices_buffer_data),
		palm_indices_buffer_data, GL_STATIC_DRAW);

	//Initialization for Rectangle/Finger VAO
	glGenVertexArrays(1, &fingerVertexArrayID[0]);
	glBindVertexArray(fingerVertexArrayID[0]);
	glGenBuffers(1, &vertexbufferfinger);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbufferfinger);
	glBufferData(GL_ARRAY_BUFFER, sizeof(finger_vertex_buffer_data), finger_vertex_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vertposition_loc, // This number must match the layout in the shader
		3, // Size
		GL_FLOAT, // Type
		GL_FALSE, // Is normalized
		0, ((void*)0));
	glEnableVertexAttribArray(vertposition_loc);
	glGenBuffers(1, &colorbufferfinger);
	glBindBuffer(GL_ARRAY_BUFFER, colorbufferfinger);
	glBufferData(GL_ARRAY_BUFFER, sizeof(finger_color_buffer_data), finger_color_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vertcolor_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertcolor_loc);
	glGenBuffers(1, &indexbufferfinger);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbufferfinger);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(finger_indices_buffer_data),
		finger_indices_buffer_data, GL_STATIC_DRAW);

	//Initialization for tip VAO
	glGenVertexArrays(1, &tipVertexArrayID[0]);
	glBindVertexArray(tipVertexArrayID[0]);
	glGenBuffers(1, &vertexbuffertip);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffertip);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tip_vertex_buffer_data), tip_vertex_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vertposition_loc, // This number must match the layout in the shader
		3, // Size
		GL_FLOAT, // Type
		GL_FALSE, // Is normalized
		0, ((void*)0));
	glEnableVertexAttribArray(vertposition_loc);
	glGenBuffers(1, &colorbuffertip);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffertip);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tip_color_buffer_data), tip_color_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vertcolor_loc, 3, GL_FLOAT, GL_FALSE, 0, ((void*)0));
	glEnableVertexAttribArray(vertcolor_loc);
	glGenBuffers(1, &indexbuffertip);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffertip);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(tip_indices_buffer_data),
		tip_indices_buffer_data, GL_STATIC_DRAW);

}

void display(){
	string input;
	string output;
	FILE *fp;
	struct Keyframe c;   // calling it "c" for "configuration"

	if (fp = fopen("animdata.bin", "rb")) {
		while (fread((void *)&c, sizeof(c), 1, fp) == 1) {

			// Clear the color and depth buffers
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			object = 'p';
			frame = 'w';
			translate(c.palm_p[0]/10000, c.palm_p[1]/10000, c.palm_p[2]/10000);
			axis = 'x';
			rotate(c.palm_q[0]);
			axis = 'y';
			rotate(c.palm_q[1]);
			axis = 'z';
			rotate(c.palm_q[1]);
			printf("The palm is at position (%.2f, %.2f, %.2f) \n", c.palm_p[0] / 100, c.palm_p[1] / 100, c.palm_p[2] / 100);
			cout << palm;
			cout << "\n";
			
			// Send our transformation to the shader
			glUniformMatrix4fv(Matrix_loc, 1, GL_FALSE, &palm[0][0]);
	
			glBindVertexArray(palmVertexArrayID[0]);
			//Draw the palm
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFF);
			glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
	
			// Send our transformation to the shader
			glUniformMatrix4fv(Matrix_loc, 1, GL_FALSE, &finger[0][0]);
		
			glBindVertexArray(fingerVertexArrayID[0]);
			//Draw the finger
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFF);
			glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
		
			// Send our transformation to the shader
			glUniformMatrix4fv(Matrix_loc, 1, GL_FALSE, &tip[0][0]);

			glBindVertexArray(tipVertexArrayID[0]);
			//Draw the tip
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex(0xFFFF);
			glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
		
			//Ask GL to execute the commands from the buffer
			glFlush();	// *** if you are using GLUT_DOUBLE, use glutSwapBuffers() instead 
			}
		fclose(fp);
		}
}

int main(int argc, char** argv){
	// For more details about the glut calls, 
	// refer to the OpenGL/freeglut Introduction handout.
	

	//Initialize the freeglut library
	glutInit(&argc, argv);

	//Specify the display mode
	glutInitDisplayMode(GLUT_RGBA);

	//Set the window size/dimensions
	glutInitWindowSize(900, 700);
	glutInitWindowPosition(200, 200);

	// Specify OpenGL version and core profile.
	// We use 3.3 in thie class, not supported by very old cards
	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutCreateWindow("swb602 - Assignment 3");

	glewExperimental = GL_TRUE;

	if (glewInit())
		exit(EXIT_FAILURE);

	init();

	glutDisplayFunc(display);

	//Transfer the control to glut processing loop.
	glutMainLoop();

	return 0;
}
