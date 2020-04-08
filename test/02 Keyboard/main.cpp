#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <glut.h>//Utilities e.g: setting camera view and projection
#include <GL/GL.h>
#include<string>
#include <cstdarg>
// Include GLM
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
struct Vector3f
{
	float x, y, z;
	Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{
	}
};

Vector3f  eye(10, 0, 10); // eye position
float lx = -1.0f, lz = -1.0f;
float horizontalAngle = 0.0;
float deltaAngle = 0.0f;
float deltaMove = 0.2f;

GLfloat	xPosOfTeaPot = 0.0f;
GLfloat yPosOfTeaPot = 0;
GLfloat zPosOfTeaPot = 0;
GLint sign = 1;//direction of moving camera

bool cubeAppeared = true;
bool triAppeared = false;
float x = -20.0f;
float y = 0.0f;
float z = 0.0f;

float xCone = -25.0f;
float yCone = 0.0f;
float zCone = 0.0f;

float score = 0.0;
bool isCrashed = false;

Vector3f lightPos(-3, 5, 5); // lightDir vector
Vector3f lightColor(1, 1, 1);
bool fullScreenMode = true; // Full-screen or windowed mode?
int windowWidth = 1920;     // Windowed mode's width
int windowHeight = 1080;     // Windowed mode's height
int windowPosX = 100;      // Windowe	d mode's top-left corner x
int windowPosY = 100;      // Windowed mode's top-left corner y
float angle = 0.0;		  // angle of rotation for the camera direction
float MaterialSh = 120;

GLfloat	rtri = 0;				// Angle For The Triangle ( NEW )
GLfloat	rquad = 0;				// Angle For The Quad ( NEW )
GLuint progShader;

GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

struct Model3D {
	GLuint TextureID;
	GLuint Texture;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint nmbuffer;
};
Model3D clase;

GLuint loadBMP_custom(const char* imagepath) {

	printf("Reading image %s\n", imagepath);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char* data;

	// Open the file
	FILE* file = fopen(imagepath, "rb");
	if (!file) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
		getchar();
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54) {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    fclose(file); return 0; }
	if (*(int*)&(header[0x1C]) != 24) { printf("Not a correct BMP file\n");    fclose(file); return 0; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

										 // Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file can be closed.
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete[] data;

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture we just created
	return textureID;
}

bool loadOBJ(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

				   // else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}
	fclose(file);
	return true;
}


void computePos(float deltaMove) {
	eye.x += deltaMove * lx * 0.1f;
	eye.z += deltaMove * lz * 0.1f;
}

void computeDir(float deltaAngle) {
	horizontalAngle += deltaAngle;
	lx = sin(horizontalAngle);
	lz = cos(horizontalAngle);
}

void changeSize(int w, int h) {
	if (h == 0)	h = 1; // Prevent a divide by zero, when window is too short
	float ratio = 1.0 * w / h;

	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// Set the correct perspective.
	gluPerspective(70, ratio, 1, 1000);//fovy, ratio, near, far

	glMatrixMode(GL_MODELVIEW);
}

void updateShaderResources()
{
	GLint loc;
	loc = glGetUniformLocation(progShader, "materialSh");
	glUniform1f(loc, MaterialSh);

	loc = glGetUniformLocation(progShader, "Eye");
	glUniform3f(loc, eye.x, eye.y, eye.z);

	loc = glGetUniformLocation(progShader, "LightPosition_worldspace");
	glUniform3f(loc, lightPos.x, lightPos.y, lightPos.z);

	loc = glGetUniformLocation(progShader, "lightColor");
	glUniform3f(loc, lightColor.x, lightColor.y, lightColor.z);

	MatrixID = glGetUniformLocation(progShader, "MVP");
	ViewMatrixID = glGetUniformLocation(progShader, "V");
	ModelMatrixID = glGetUniformLocation(progShader, "M");
}

float randomPos(float min, float max) {
	float range = max - min + 1;
	float num = rand() % int(range) + min;
	return num;
}

void drawTeapot()
{
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(xPosOfTeaPot, yPosOfTeaPot, zPosOfTeaPot);
	/*if (xPosOfTeaPot < 10)
		xPosOfTeaPot += 0.01f;
	else
		xPosOfTeaPot = -10.0f;*/
	if (deltaMove > 0) {
		xPosOfTeaPot -= deltaMove/10;
	}
	//glRotatef(rquad, 0.0f, 1.0f, 0.0f);					// Rotate The Quad On The X axis ( NEW )
	//rquad += 0.05f;										// Decrease The Rotation Variable For The Quad ( NEW )
	glColor3f(0.0, 1.0, 0.0);
	//glutSolidTeapot(2.0);
	glutWireTeapot(2.0);
	//glutSolidSphere(1, 8, 8);
	//glColor3f(1.0, 1.0, 0.0);
	//glutWireTeapot(2.0);
	glEnd();
	glPopMatrix();
}

void drawCube(float x, float y, float z)
{
	glPushMatrix();
	//glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(x,y,z);
	// Move Right 1.5 Units And Into The Screen 7.0
	/*glRotatef(randomPos(45, 90),0,0,1.0f);		*/			// Rotate The Quad On The X axis ( NEW )
	//rquad-=0.015f;										// Decrease The Rotation Variable For The Quad ( NEW )
	glBegin(GL_QUADS);									// Draw A Quad
	glColor3f(0.0f, 1.0f, 0.0f);						// Set The Color To Green
	glVertex3f(1.0f, 1.0f, -1.0f);					// Top Right Of The Quad (Top)
	glVertex3f(-1.0f, 1.0f, -1.0f);					// Top Left Of The Quad (Top)
	glVertex3f(-1.0f, 1.0f, 1.0f);					// Bottom Left Of The Quad (Top)
	glVertex3f(1.0f, 1.0f, 1.0f);					// Bottom Right Of The Quad (Top)
	glColor3f(1.0f, 0.5f, 0.0f);						// Set The Color To Orange
	glVertex3f(1.0f, -1.0f, 1.0f);					// Top Right Of The Quad (Bottom)
	glVertex3f(-1.0f, -1.0f, 1.0f);					// Top Left Of The Quad (Bottom)
	glVertex3f(-1.0f, -1.0f, -1.0f);					// Bottom Left Of The Quad (Bottom)
	glVertex3f(1.0f, -1.0f, -1.0f);					// Bottom Right Of The Quad (Bottom)
	glColor3f(1.0f, 0.0f, 0.0f);						// Set The Color To Red
	glVertex3f(1.0f, 1.0f, 1.0f);					// Top Right Of The Quad (Front)
	glVertex3f(-1.0f, 1.0f, 1.0f);					// Top Left Of The Quad (Front)
	glVertex3f(-1.0f, -1.0f, 1.0f);					// Bottom Left Of The Quad (Front)
	glVertex3f(1.0f, -1.0f, 1.0f);					// Bottom Right Of The Quad (Front)
	glColor3f(1.0f, 1.0f, 0.0f);						// Set The Color To Yellow
	glVertex3f(1.0f, -1.0f, -1.0f);					// Top Right Of The Quad (Back)
	glVertex3f(-1.0f, -1.0f, -1.0f);					// Top Left Of The Quad (Back)
	glVertex3f(-1.0f, 1.0f, -1.0f);					// Bottom Left Of The Quad (Back)
	glVertex3f(1.0f, 1.0f, -1.0f);					// Bottom Right Of The Quad (Back)
	glColor3f(0.0f, 0.0f, 1.0f);						// Set The Color To Blue
	glVertex3f(-1.0f, 1.0f, 1.0f);					// Top Right Of The Quad (Left)
	glVertex3f(-1.0f, 1.0f, -1.0f);					// Top Left Of The Quad (Left)
	glVertex3f(-1.0f, -1.0f, -1.0f);					// Bottom Left Of The Quad (Left)
	glVertex3f(-1.0f, -1.0f, 1.0f);					// Bottom Right Of The Quad (Left)
	glColor3f(1.0f, 0.0f, 1.0f);						// Set The Color To Violet
	glVertex3f(1.0f, 1.0f, -1.0f);					// Top Right Of The Quad (Right)
	glVertex3f(1.0f, 1.0f, 1.0f);					// Top Left Of The Quad (Right)
	glVertex3f(1.0f, -1.0f, 1.0f);					// Bottom Left Of The Quad (Right)
	glVertex3f(1.0f, -1.0f, -1.0f);					// Bottom Right Of The Quad (Right)
	glEnd();
	glPopMatrix();
	// Done Drawing The Quad
}

void drawTriangle() {
	glPushMatrix();
	//glLoadIdentity();
	glRotatef(rquad, 0.0f, 1.0f, 0.0f);	
	glMatrixMode(GL_MODELVIEW);
	glColor3f(1.0, 0.5, 0);
	glBegin(GL_TRIANGLES);
	glVertex3f(-0.5, -0.5, 0.0);
	glVertex3f(0.5, 0.0, 0.0);
	glVertex3f(0.0, 0.5, 0.0);
	glEnd();
	glPopMatrix();
}

void drawCone(float x, float y, float z) {
	glPushMatrix();               // Reset the model-view matrix
	glTranslatef(x, y, z);  // Move left and into the screen

	glBegin(GL_TRIANGLES);           // Begin drawing the pyramid with 4 triangles
	 // Front
	glColor3f(1.0f, 0.0f, 0.0f);     // Red
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);     // Green
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);     // Blue
	glVertex3f(1.0f, -1.0f, 1.0f);

	// Right
	glColor3f(1.0f, 0.0f, 0.0f);     // Red
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);     // Blue
	glVertex3f(1.0f, -1.0f, 1.0f);
	glColor3f(0.0f, 1.0f, 0.0f);     // Green
	glVertex3f(1.0f, -1.0f, -1.0f);

	// Back
	glColor3f(1.0f, 0.0f, 0.0f);     // Red
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);     // Green
	glVertex3f(1.0f, -1.0f, -1.0f);
	glColor3f(0.0f, 0.0f, 1.0f);     // Blue
	glVertex3f(-1.0f, -1.0f, -1.0f);

	// Left
	glColor3f(1.0f, 0.0f, 0.0f);       // Red
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);       // Blue
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glColor3f(0.0f, 1.0f, 0.0f);       // Green
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glEnd();   // Done drawing the pyramid
	glPopMatrix();
}

void drawAxes() {
	glPushMatrix();
	glColor3f(1.0, 0.0, 0.0); // red x
	glBegin(GL_LINES);
	// x aix

	glVertex3f(-4.0, 0.0f, 0.0f);
	glVertex3f(4.0, 0.0f, 0.0f);

	// arrow
	glVertex3f(4.0, 0.0f, 0.0f);
	glVertex3f(3.0, 1.0f, 0.0f);

	glVertex3f(4.0, 0.0f, 0.0f);
	glVertex3f(3.0, -1.0f, 0.0f);
	glEnd();
	glFlush();

	// y 
	glColor3f(0.0, 1.0, 0.0); // green y
	glBegin(GL_LINES);
	glVertex3f(0.0, -4.0f, 0.0f);
	glVertex3f(0.0, 4.0f, 0.0f);

	// arrow
	glVertex3f(0.0, 4.0f, 0.0f);
	glVertex3f(1.0, 3.0f, 0.0f);

	glVertex3f(0.0, 4.0f, 0.0f);
	glVertex3f(-1.0, 3.0f, 0.0f);
	glEnd();
	glFlush();

	// z 
	glColor3f(0.0, 0.0, 1.0); // blue z
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0f, -4.0f);
	glVertex3f(0.0, 0.0f, 4.0f);

	// arrow
	glVertex3f(0.0, 0.0f, 4.0f);
	glVertex3f(0.0, 1.0f, 3.0f);

	glVertex3f(0.0, 0.0f, 4.0f);
	glVertex3f(0.0, -1.0f, 3.0f);
	glEnd();
	glFlush();
	glPopMatrix();
}

void displayScore(int x, int y, float r, float g, float b, char *string) {
	glColor3f(r, g, b);
	glRasterPos2f(x, y);
	int len, i;
	len = (int)strlen(string);
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
	}
}

void display() {
	if (cubeAppeared) {
		x -= randomPos(20.0, 45.0);
		z = randomPos(-3.0, 3.0);
		drawCube(x, 0, z);
	}
	
}

void displayCone() {
	if (triAppeared) {
		xCone -= randomPos(15.0, 25.0);
		zCone = randomPos(-3.0, 3.0);
		drawCube(xCone, 0.0, zCone);
	}
	
}

void nextLevel() {
	switch (int(score))
	{
	case 200:
		deltaMove += 0.02f;
		break;
	case 1000:
		deltaMove += 0.04f;
		break;
	case 1500:
		deltaMove += 0.05f;
		break;
	case 2000:
		deltaMove += 0.06f;
		break;
	default:
		break;
	}
}

//display random object on the road
void getCrashed(int xCube, int zCube) {
	if (int(xPosOfTeaPot) - int(xCube) - 2 == 0 && int(zPosOfTeaPot)-int(zCube) == 0) {
		deltaMove = 0.0f;
	}
	else {
		if (deltaMove > 0) {
			score += 0.1;
			nextLevel();
		}
		printf("score: %f\n", score);
		
	}
}

void drawObjects(GLvoid)									// Here's Where We Do All The Drawing
{
	drawAxes();
	//drawTriangles();	
	// Done Drawing The Pyramid
	drawTeapot();
	drawCube(x, y, z);
	if (xPosOfTeaPot < x-10) {
		cubeAppeared = !cubeAppeared;
		display(); 
	}
	

	getCrashed(x, z);
	
	//std::string s = std::to_string(score);
	//char const* pchar = s.c_str();
 //	displayScore(15, 20, 124.0, 252.0, 0.0, pchar);
}

void renderScene(void) {
	if (deltaMove)
		computePos(deltaMove);
	if (deltaAngle)
		computeDir(deltaAngle);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	drawObjects();
	
	glutSwapBuffers();
	glLoadIdentity();
	gluLookAt(eye.x, 2.0f, 0.0, eye.x + 18*lx, 0.0f, 0, 0.0, 1.0, 0.0);//eye, at, up
	/*Eye.y += sign * 0.01f;
	if (Eye.y > 10)
		sign = -1;
	else if(Eye.y < -10)
		sign = 1;*/
}

/* Callback handler for normal-key event */
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:     // ESC key
		exit(0);
		break;
	}
}

void moveCam(int key, int xx, int yy) {
	switch (key) {
	case GLUT_KEY_LEFT: deltaAngle = -0.005f; break;
	case GLUT_KEY_RIGHT: deltaAngle = 0.005f; break;
	case GLUT_KEY_UP: deltaMove = 0.5f; break;
	case GLUT_KEY_DOWN: deltaMove = -0.5f; break;
	
	}
}

void releaseMoveCam(int key, int x, int y) {

	switch (key) {
	case GLUT_KEY_LEFT:
	case GLUT_KEY_RIGHT: deltaAngle = 0.0f; break;
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN: deltaMove = 0; break;
	}
}

void moveTeaPot(unsigned char key, int xx, int yy) {
	switch (key) {
		case 'a':
			if (zPosOfTeaPot <= 5) {
				zPosOfTeaPot += deltaMove+0.2f;

			}
			break;
		case 'd': 
			if (-5 <= zPosOfTeaPot) {
				zPosOfTeaPot -= deltaMove + 0.2f;
			}
			break;
		default: break;

	}
}

void initGL(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);//The mode parameter is a Boolean combination
	glutInitWindowPosition(100, 100);//-1,-1 up to the windows manager
	glutInitWindowSize(1024, 720);
	glutCreateWindow("Setup MVP matrices");


	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	
	glutReshapeFunc(changeSize);

	glutIgnoreKeyRepeat(0);
	//glutSpecialFunc(moveCam); // Register callback handler for special-key event
	//glutSpecialUpFunc(releaseMoveCam);
	glutKeyboardFunc(keyboard);   // Register callback handler for special-key event
	// register callback handler for keyboard pressing
	glutKeyboardFunc(moveTeaPot);

	glEnable(GL_DEPTH_TEST);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_CULL_FACE);
}

int main(int argc, char** argv)
{
	initGL(argc, argv);
	glutMainLoop();//says: we're ready to get in the event processing loop
	
	return 0;
}
