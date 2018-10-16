#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include <cstdlib>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

struct color{
	float r, g, b;
};
typedef struct color color;
bool operator==(const color& lhs, const color& rhs)
{
    return (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b);
}
struct Object2D {
	string name;
	float x, y;
	float height, width;
	float angle;
	color objcolor;
	VAO *objectvao;
	int active;
};

typedef struct Object2D Object2D;

map <string, Object2D> bricks;
map <string, Object2D> gunparts;
map <string, Object2D> beam;
map <string, Object2D> mirrors;
map <string, Object2D> buckets;
map <string, Object2D> scoreparts;
map <string, Object2D> display;
map <string, Object2D> lives;
map <string, Object2D> highlights;

color red = {1, 0, 0};
color blue = {0, 1, 1};
color black = {0, 0, 0};
color grey = {168.0/255.0, 168.0/255.0, 168.0/255.0};
color green = {0.5, 0, 1};
color yellow = {1, 1, 0};
int brickspeed;
int beamind;
int totalscore;
double last_beam_time;
int life;
int gameover;
float zoomamount;
float dx, dy;
int leftclicked = 0;
int rightclicked = 0;
int clickedobj = -1;
double curmousex = 0;
double curmousey = 0;
double panx = 0;
double pany = 0;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("\n\n\n\nCompiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	//fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

		// Create Vertex Array Object
		// Should be done after CreateWindow and before any other GL calls
		glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
		glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
		glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

		glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
		glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
		glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
		glVertexAttribPointer(
													0,                  // attribute 0. Vertices
													3,                  // size (x,y,z)
													GL_FLOAT,           // type
													GL_FALSE,           // normalized?
													0,                  // stride
													(void*)0            // array buffer offset
													);

		glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
		glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
		glVertexAttribPointer(
													1,                  // attribute 1. Color
													3,                  // size (r,g,b)
													GL_FLOAT,           // type
													GL_FALSE,           // normalized?
													0,                  // stride
													(void*)0            // array buffer offset
													);

		return vao;
	}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
	struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
	{
		GLfloat* color_buffer_data = new GLfloat [3*numVertices];
		for (int i=0; i<numVertices; i++) {
			color_buffer_data [3*i] = red;
			color_buffer_data [3*i + 1] = green;
			color_buffer_data [3*i + 2] = blue;
		}

		return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
	}

/* Render the VBOs handled by VAO */
	void draw3DObject (struct VAO* vao)
	{
		// Change the Fill Mode for this object
		glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

		// Bind the VAO to use
		glBindVertexArray (vao->VertexArrayID);

		// Enable Vertex Attribute 0 - 3d Vertices
		glEnableVertexAttribArray(0);
		// Bind the VBO to use
		glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

		// Enable Vertex Attribute 1 - Color
		glEnableVertexAttribArray(1);
		// Bind the VBO to use
		glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

		// Draw the geometry !
		glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
	}

/**************************
 * Customizable functions *
 **************************/



/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
VAO *rectangle;
void createRectangle (string name, color objcolor, float angle, float x, float y, float height, float width, string objclass)
{
	//cout << objcolor.r << objcolor.g << objcolor.b;
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-width/2, height/2, 0, // vertex 1
		-width/2, -height/2, 0, // vertex 2
		width/2, height/2, 0, // vertex 3

		width/2, height/2, 0, // vertex 3
		width/2, -height/2, 0, // vertex 4
		-width/2, -height/2, 0,  // vertex 1
	};

	GLfloat color_buffer_data [] = {
		objcolor.r, objcolor.g, objcolor.b, // color 1
		objcolor.r, objcolor.g, objcolor.b, // color 2
		objcolor.r, objcolor.g, objcolor.b, // color 3

		objcolor.r, objcolor.g, objcolor.b, // color 3
		objcolor.r, objcolor.g, objcolor.b, // color 4
		objcolor.r, objcolor.g, objcolor.b, // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
	Object2D obj = {};
	obj.objectvao = rectangle;
	obj.name = name;
	obj.x = x;
	obj.y = y;
	obj.height = height;
	obj.width = width;
	obj.angle = angle;
	obj.objcolor = objcolor;
	obj.active = 1;
	if (objclass == "buckets")
	buckets[name] = obj;
	else if (objclass == "bricks")
	bricks[name] = obj;
	else if (objclass == "gunparts")
	gunparts[name] = obj;
	else if (objclass == "scoreparts")
	scoreparts[name] = obj;
	else if (objclass == "mirrors")
	mirrors[name] = obj;
	else if (objclass == "display")
	display[name] = obj;
	else if (objclass == "lives")
	lives[name] = obj;
	else if (objclass == "highlights")
	highlights[name] = obj;
	else if (objclass == "beam"){
		obj.active = 0;
		beam[name] = obj;
	}
}
void moveGunUporDown(int dir)
{
	for (map <string, Object2D> :: iterator it = gunparts.begin(); it != gunparts.end(); it++){
		string name = it->first;
		if(dir == 1 && gunparts[name].y <= 200)
			gunparts[name].y += 10;
		else if (dir == -1 && gunparts[name].y >= -150)
			gunparts[name].y -= 10;
		
	}
}
void tiltGunUporDown(int dir)
{
	for (map <string, Object2D> :: iterator it = gunparts.begin(); it != gunparts.end(); it++){
		string name = it->first;
		if(dir == 1){
			if((name == "gunArm" && gunparts[name].angle < 60.0) || (name == "gunBase2" && gunparts[name].angle < 105.0))
				gunparts[name].angle += 10.0;	
		}
		if(dir == -1){
			if((name == "gunArm" && gunparts[name].angle > -60.0) || (name == "gunBase2" && gunparts[name].angle > -15.0))
				gunparts[name].angle -= 10.0;	
		}
		
	}
}
void moveBucket(int color, int dir)
{
	if(color == 0 && dir == 1){
		if(buckets["redBucket"].x <= 340.0)
		buckets["redBucket"].x += 10;
	}
	if(color == 0 && dir == -1){
		if(buckets["redBucket"].x >= -340.0)
			buckets["redBucket"].x -= 10;
	}
	if(color == 1 && dir == 1){
		if(buckets["blueBucket"].x <= 340.0)
		buckets["blueBucket"].x += 10;
	}
	if(color == 1 && dir == -1){
		if(buckets["blueBucket"].x >= -340.0)
			buckets["blueBucket"].x -= 10;
	}
}
void firegun()
{
	if(gunparts["gunArm"].active == 0)
	{
		last_beam_time = glfwGetTime();
		char temp[10];
		snprintf(temp, 10, "%d", beamind);
		string str (temp);
		str = "beam" + str;
		createRectangle (str, red, 0.0, -375.0, 0, 1.0, 30.0, "beam");
		beam[str].angle = gunparts["gunArm"].angle;
		beam[str].x = gunparts["gunArm"].x - 375.0 + 70.0 * cos (beam[str].angle * M_PI/180.0);
		beam[str].y = gunparts["gunArm"].y + 70.0 * sin (beam[str].angle * M_PI/180.0);
		gunparts["gunArm"].active = 1;
		beam[str].active = 1;
		beamind ++;
	}

}
int bottomCollision (Object2D a, Object2D b)
{
	if((a.y - (a.height)/2.0) < (b.y + (b.height)/2.0)){
		if(((a.x + (a.width)/2.0) < (b.x + (b.width)/2.0)) && ((a.x - (a.width)/2.0) > (b.x - (b.width)/2.0)))
			return 1;
	}
	return 0;
}
void zoom(int size)
{
	if(size == 1)
		zoomamount *= 1.1;
	else if(zoomamount != 1.0)
		zoomamount /= 1.1;

	if(400.0/zoomamount + dx > 400.0)
		dx = 400.0 - 400.0/zoomamount;
	if(-400.0/zoomamount + dx < -400.0)
		dx = -400.0 + 400.0/zoomamount;
	if(300.0/zoomamount + dy > 300.0)
		dy = 300.0 - 300.0/zoomamount;
	if(-300.0/zoomamount + dy < -300.0)
		dy = -300.0 + 300.0/zoomamount;
	Matrices.projection = glm::ortho(-400.0f/zoomamount + dx, 400.0f/zoomamount + dx, -300.0f/zoomamount + dy, 300.0f/zoomamount + dy, 0.1f, 500.0f);
}
void pan(int dir)
{
	if(dir == 1)
		dx += 10;
	else if(dir == -1)
		dx -= 10;
	else if(dir == 2)
		dy += 10;
	else if(dir == -2)
		dy -= 10;

	if(400.0/zoomamount + dx > 400.0)
		dx = 400.0 - 400.0/zoomamount;
	if(-400.0/zoomamount + dx < -400.0)
		dx = -400.0 + 400.0/zoomamount;
	if(300.0/zoomamount + dy > 300.0)
		dy = 300.0 - 300.0/zoomamount;
	if(-300.0/zoomamount + dy < -300.0)
		dy = -300.0 + 300.0/zoomamount;

	Matrices.projection = glm::ortho(-400.0f/zoomamount + dx, 400.0f/zoomamount + dx, -300.0f/zoomamount + dy, 300.0f/zoomamount + dy, 0.1f, 500.0f);

}
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	 // Function is called first on GLFW_PRESS.
	glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
	if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		moveBucket(0, 1);
	if ((glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		moveBucket(0, -1);
	if ((glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		moveBucket(1, 1);
	if ((glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		moveBucket(1, -1);
	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_A:
			tiltGunUporDown(1);
			break;
			case GLFW_KEY_D:
			tiltGunUporDown(-1);
			break;
			case GLFW_KEY_S:
			moveGunUporDown(1);
			break;
			case GLFW_KEY_F:
			moveGunUporDown(-1);
			break;
			case GLFW_KEY_SPACE:
			firegun();
			break;
			case GLFW_KEY_N:
			if(brickspeed < 7.0)
				brickspeed ++;
			break;
			case GLFW_KEY_M:
			if(brickspeed > 1.0)
				brickspeed --;
			break;
			case GLFW_KEY_UP:
			zoom(1);
			break;
			case GLFW_KEY_DOWN:
			zoom(-1);
			break;
			case GLFW_KEY_RIGHT:
			pan(1);
			break;
			case GLFW_KEY_LEFT:
			pan(-1);
			break;
			case GLFW_KEY_K:
			pan(2);
			break;
			case GLFW_KEY_L:
			pan(-2);
			break;
			/*case GLFW_KEY_H:
			moveBucket(0, 1);
			break;
			case GLFW_KEY_J:
			moveBucket(0, -1);
			break;
			case GLFW_KEY_K:
			moveBucket(1, 1);
			break;
			case GLFW_KEY_L:
			moveBucket(1, -1);
			break;*/
			default:
			break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
			quit(window);
			break;
			default:
			break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
		quit(window);
		break;
		default:
		break;
	}
}
void mouseClicked(GLFWwindow* window)
{
	leftclicked = 1;
	glfwGetCursorPos(window, &curmousex, &curmousey);
	curmousex -= 400.0;
	curmousey = 300.0 - curmousey ;
	if((curmousex < buckets["redBucket"].x + (buckets["redBucket"].width)/2.0)
		&& (curmousex > buckets["redBucket"].x - (buckets["redBucket"].width)/2.0)
		&& (curmousey < buckets["redBucket"].y + (buckets["redBucket"].height)/2.0)
		&& (curmousey > buckets["redBucket"].y - (buckets["redBucket"].height)/2.0))
		clickedobj = 0;
	else if((curmousex < buckets["blueBucket"].x + (buckets["blueBucket"].width)/2.0)
		&& (curmousex > buckets["blueBucket"].x - (buckets["blueBucket"].width)/2.0)
		&& (curmousey < buckets["blueBucket"].y + (buckets["blueBucket"].height)/2.0)
		&& (curmousey > buckets["blueBucket"].y - (buckets["blueBucket"].height)/2.0))
		clickedobj = 1;
	else if((curmousex < -375.0 + gunparts["gunBase1"].x + (gunparts["gunBase1"].width))
			&& (curmousex > -375.0 + gunparts["gunBase1"].x - (gunparts["gunBase1"].width)/2.0)
			&& (curmousey < gunparts["gunBase1"].y + (gunparts["gunBase1"].height))
			&& (curmousey > gunparts["gunBase1"].y - (gunparts["gunBase1"].height))){
				clickedobj = 2;
	}
	else {
		float m = (curmousey - gunparts["gunArm"].y)/(curmousex - gunparts["gunArm"].x + 375.0);
		float angle = atan (m);
		angle = angle * 180.0/M_PI;
		if(angle > -60.0 && angle < 60.0){
			gunparts["gunArm"].angle = angle;
			gunparts["gunBase2"].angle = angle + 45.0;
			clickedobj = 3;
		}
	}

}
void mousescroll(GLFWwindow* window, double dx, double dy)
{
	if(dy > 0)
	zoom(1);
	
	else if (dy < 0)
		zoom(-1);
}
void mouseReleased(GLFWwindow* window)
{
	if(clickedobj == 3)
		firegun();
	leftclicked = 0;
	clickedobj = -1;
}
/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
		if (action == GLFW_PRESS){
			mouseClicked(window);
		}
		if (action == GLFW_RELEASE){
			mouseReleased(window);
		}
		break;
		case GLFW_MOUSE_BUTTON_RIGHT:
		if (action == GLFW_PRESS){
			rightclicked = 1;
			glfwGetCursorPos(window, &panx, &pany);
			panx -= 400.0;
			pany = 300.0 - pany ;
		}
		if (action == GLFW_RELEASE){
			rightclicked = 0;
		}
		break;
		default:
		break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

// set the projection matrix as perspective
/* glMatrixMode (GL_PROJECTION);
	 glLoadIdentity ();
	 gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	//Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

	// Ortho projection for 2D views
	Matrices.projection = glm::ortho(-400.0f, 400.0f, -300.0f, 300.0f, 0.1f, 500.0f);
}
/* Render the scene with openGL */
/* Edit this function according to your assignment */
glm::mat4 VP;
glm::mat4 MVP;
float camera_rotation_angle = 90.0;
void drawDig(int dig, int place)
{
	float x, y;
	if(place == 0){
		if(dig == -1 || dig == -2 || dig == -3){
			x = 30.0;
			y = 0.0;
		}
		else {
			x = 370.0;
			y = 250.0;
		}
	}
	else if(place == 1){
		if(dig == -1 || dig == -2 || dig == -3){
			x = 0.0;
			y = 0.0;
		}
		else {
			x = 340.0;
			y = 250.0;
		}
	}
	else if(place == 2){
		if(dig == -1 || dig == -2 || dig == -3){
			x = -30.0;
			y = 0.0;
		}
		else {
			x = 310.0;
			y = 250.0;
		}
	}
	if(dig == 0 || dig == 2 || dig == 3 || dig == 5 || dig == 6 || dig == 7 || dig == 8 || dig == 9 || dig == -3){
		string name;
		if(place == 0) name = "top";
		else if(place == 1) name = "top1";
		else if(place == 2) name = "top2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 0 || dig == 1 || dig == 2 || dig == 3 || dig == 4 || dig == 7 || dig == 8 || dig == 9 || dig == -1){
		string name;
		if(place == 0) name = "topright";
		else if(place == 1) name = "topright1";
		else if(place == 2) name = "topright2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 0 || dig == 4 || dig == 5 || dig == 6 || dig == 8 || dig == 9 || dig == -3){
		string name;
		if(place == 0) name = "topleft";
		else if(place == 1) name = "topleft1";
		else if(place == 2) name = "topleft2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 2 || dig == 3 || dig == 4 || dig == 5 || dig == 6 || dig == 8 || dig == 9 || dig == -1 || dig == -2 || dig == -3){
		string name;
		if(place == 0) name = "middle";
		else if(place == 1) name = "middle1";
		else if(place == 2) name = "middle2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 0 || dig == 1 || dig == 3 || dig == 4 || dig == 5 || dig == 6 || dig == 7 || dig == 8 || dig == 9 || dig == -2 || dig == -1){
		string name;
		if(place == 0) name = "botright";
		else if(place == 1) name = "botright1";
		else if(place == 2) name = "botright2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 0 || dig == 2 || dig == 6 || dig == 8 || dig == -1 || dig == -2 || dig == -3){
		string name;
		if(place == 0) name = "botleft";
		else if(place == 1) name = "botleft1";
		else if(place == 2) name = "botleft2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
	if(dig == 0 || dig == 2 || dig == 3 || dig == 5 || dig == 6 || dig == 8 || dig == 9 || dig == -1 || dig == -3){
		string name;
		if(place == 0) name = "bottom";
		else if(place == 1) name = "bottom1";
		else if(place == 2) name = "bottom2";
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(x + display[name].x, y + display[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(display[name].objectvao);
	}
}
void draw (GLFWwindow* window)
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	//Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
	VP = Matrices.projection * Matrices.view;
	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	  // MVP = Projection * View * Model

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	/*glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
	glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
	Matrices.model *= triangleTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M*/

	//  Don't change unless you are sure!!
	/*glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);*/

	// draw3DObject draws the VAO given to it using current MVP matrix
	//draw3DObject(triangle);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
	/*Matrices.model = glm::mat4(1.0f);*/

	
	//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	//glm::mat4 scaleRectangle = glm::scale(glm::vec3(2.0f, 2.0f ,2.0f));
	//Draw bricks
	if(gameover){
		drawDig(-3, 2);
		drawDig(-2, 1);
		drawDig(-1, 0);
		return;
	}
	if(rightclicked){
		glfwGetCursorPos(window, &curmousex, &curmousey);
		curmousex -= 400.0;
		curmousey = 300.0 - curmousey;
		double dx1 = curmousex - panx;
		double dy1 = curmousey - pany;
		
		dx -= dx1;
		dy -= dy1;

		if(400.0/zoomamount + dx > 400.0)
			dx = 400.0 - 400.0/zoomamount;
		if(-400.0/zoomamount + dx < -400.0)
			dx = -400.0 + 400.0/zoomamount;
		if(300.0/zoomamount + dy > 300.0)
			dy = 300.0 - 300.0/zoomamount;
		if(-300.0/zoomamount + dy < -300.0)
			dy = -300.0 + 300.0/zoomamount;

		Matrices.projection = glm::ortho(-400.0f/zoomamount + dx, 400.0f/zoomamount + dx, -300.0f/zoomamount + dy, 300.0f/zoomamount + dy, 0.1f, 500.0f);
	}
	if(leftclicked){
		glfwGetCursorPos(window, &curmousex, &curmousey);
		curmousex -= 400.0;
		curmousey = 300.0 - curmousey;
		if(clickedobj == 0){
			if(curmousex < 400.0 && curmousex > -400.0)
				buckets["redBucket"].x = curmousex;
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateRectangle = glm::translate (glm::vec3(buckets["redBucket"].x, buckets["redBucket"].y, 0));        // glTranslatef
			Matrices.model *= (translateRectangle /** rotateRectangle * scaleRectangle*/);

			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(highlights["redBucketLight"].objectvao);
		}
		else if(clickedobj == 1){
			if(curmousex < 400.0 && curmousex > -400.0)
				buckets["blueBucket"].x = curmousex;
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateRectangle = glm::translate (glm::vec3(buckets["blueBucket"].x, buckets["blueBucket"].y, 0));        // glTranslatef
			Matrices.model *= (translateRectangle /** rotateRectangle * scaleRectangle*/);

			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(highlights["blueBucketLight"].objectvao);
		}
		else if(clickedobj == 2){
			if(curmousey <= 200.0 && curmousey >= -150.0){
				for (map <string, Object2D> :: iterator it = gunparts.begin(); it != gunparts.end(); it++){
				string name = it->first;
				gunparts[name].y = curmousey;
				}
			}
		}
	}
	if (!bricks.empty()){
		for (map <string, Object2D> :: iterator it = bricks.begin(); it != bricks.end(); it++){
			string name = it->first;
			if(bricks[name].active == 2) continue;
			//Transformations
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 translateRectangle = glm::translate (glm::vec3(bricks[name].x, bricks[name].y, 0));        // glTranslatef
			Matrices.model *= (translateRectangle /** rotateRectangle * scaleRectangle*/);

			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(bricks[name].objectvao);
			bricks[name].y -= brickspeed;
			if(bricks[name].active == 1){
				if(bricks[name].y < -240.0){
					string brickcolor;
					if(bricks[name].objcolor == blue) brickcolor = "blue";
					else if(bricks[name].objcolor == red) brickcolor = "red";
					else if(bricks[name].objcolor == black) brickcolor = "black";
					
					if(brickcolor == "red"){
						if(bottomCollision(bricks[name], buckets["redBucket"]) == 1){
							totalscore += 10;
							bricks[name].active = 2;
						}
						else
							bricks[name].active = 0;
					}
					if(brickcolor == "blue"){
						if(bottomCollision(bricks[name], buckets["blueBucket"]) == 1){
							totalscore += 10;
							bricks[name].active = 2;
						}
						else
							bricks[name].active = 0;
					}
					if(brickcolor == "black"){
						if(bottomCollision(bricks[name], buckets["redBucket"]) == 1 
							|| bottomCollision(bricks[name], buckets["blueBucket"]) == 1){
							bricks[name].active = 2;
							gameover = 1;
						}
						else
							bricks[name].active = 0;
					}
				}
				if(bricks[name].y < - 350.0)
					bricks[name].active = 2;
				else {
					for (map <string, Object2D> :: iterator it1 = beam.begin(); it1 != beam.end(); it1++){
						string beamname = it1->first;
						if(beam[beamname].active == 0)
							continue;
						string brickcolor;
						if(bricks[name].objcolor == blue) brickcolor = "blue";
						else if(bricks[name].objcolor == red) brickcolor = "red";
						else if(bricks[name].objcolor == black) brickcolor = "black";
						if((bricks[name].x + (bricks[name].width)/2.0 > beam[beamname].x + 15.0 * cos (beam[beamname].angle * M_PI/180.0)/*(beam[beamname].width)/2.0*/) 
							&& (bricks[name].x - (bricks[name].width)/2.0 < beam[beamname].x + 15.0 * cos (beam[beamname].angle * M_PI/180.0)/*(beam[beamname].width)/2.0*/)
							&& (bricks[name].y + (bricks[name].height)/2.0 > beam[beamname].y + 15.0 * sin (beam[beamname].angle * M_PI/180.0)/*(beam[beamname].height)/2.0*/)
							&& (bricks[name].y - (bricks[name].height)/2.0 < beam[beamname].y + 15.0 * sin (beam[beamname].angle * M_PI/180.0)/*(beam[beamname].height)/2.0*/)){
							bricks[name].active = 2;
							beam[beamname].active = 0;
							if(brickcolor == "black")
								totalscore += 10;
							else {
								life -= 1;
								totalscore -= 5;
								if(totalscore < 0)
									totalscore = 0;
								if(life == 0)
									gameover = 1;
							}
						}	
					}			
				}
			}
		}
	}
	//Draw Beam
	if(!beam.empty()){
		for (map <string, Object2D> :: iterator it1 = beam.begin(); it1 != beam.end(); it1++){
			string beamname = it1->first;
			if(beam[beamname].active == 0) continue;
			Matrices.model = glm::mat4(1.0f);
			glm::mat4 rotateRectangle = glm::rotate((float)(beam[beamname].angle * M_PI/180.0f), glm::vec3(0,0,1));
			glm::mat4 translateRectangle = glm::translate (glm::vec3(beam[beamname].x, beam[beamname].y, 0));        // glTranslatef
			Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);

			MVP = VP * Matrices.model;
			glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
			draw3DObject(beam[beamname].objectvao);
			beam[beamname].x += 10.0 * cos(beam[beamname].angle * M_PI/180.0f);
			beam[beamname].y += 10.0 * sin(beam[beamname].angle * M_PI/180.0f);
			if(beam[beamname].x > 400.0 || beam[beamname].x < -400.0 || beam[beamname].y > 300.0 || 
				beam[beamname].y < -300.0){
				beam[beamname].active = 0;
				break;
			}
			for (map <string, Object2D> :: iterator it = mirrors.begin(); it != mirrors.end(); it++){
				string name = it->first;
				float m = tan (mirrors[name].angle * M_PI/180);
				float perpd = fabs(m * (beam[beamname].x + 15.0 * cos(beam[beamname].angle * M_PI/180.0)) - (beam[beamname].y + 15.0 * sin (beam[beamname].angle * M_PI/180.0)) + mirrors[name].y - m * mirrors[name].x)/sqrt(pow(m, 2.0) + 1.0);
				if(perpd < 10.0/sqrt(2.0)){
					float d = sqrt(pow((beam[beamname].x + 15.0 * cos(beam[beamname].angle * M_PI/180.0)) - mirrors[name].x, 2) + pow((beam[beamname].y + 15.0 * sin (beam[beamname].angle * M_PI/180.0)) - mirrors[name].y, 2));
					if(d < 10.0/sqrt(2.0) + 15.0){
						float approachangle = fmod(beam[beamname].angle, 360);
						if(approachangle < 0.0)
							approachangle += 360.0;
						if(mirrors[name].angle == 45.0){
							if(approachangle > 45.0 && approachangle < 225.0){
								beam[beamname].active = 0;
								break;
							}
						}
						else if(mirrors[name].angle == 135.0){
							if(approachangle > 135.0 && approachangle < 315.0){
								beam[beamname].active = 0;
								break;
							}
						}
						else if(mirrors[name].angle == 315.0){
							if(approachangle > 315.0 || approachangle < 135.0){
								beam[beamname].active = 0;
								break;
							}
						}
						beam[beamname].angle = 2.0 * mirrors[name].angle - beam[beamname].angle;
						break;
					}
				}
				
			}
		}
	}
	//Draw mirrors
	for (map <string, Object2D> :: iterator it = mirrors.begin(); it != mirrors.end(); it++){
		string name = it->first;
		//Transformations
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 rotateRectangle = glm::rotate((float)(mirrors[name].angle * M_PI/180.0f), glm::vec3(0,0,1));
		glm::mat4 translateRectangle = glm::translate (glm::vec3(mirrors[name].x, mirrors[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(mirrors[name].objectvao);
	}
	//Draw Buckets
	for (map <string, Object2D> :: iterator it = buckets.begin(); it != buckets.end(); it++){
		string name = it->first;
		//Transformations
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(buckets[name].x, buckets[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle /** rotateRectangle * scaleRectangle*/);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(buckets[name].objectvao);
	}
	//Draw Gun
	for (map <string, Object2D> :: iterator it = gunparts.begin(); it != gunparts.end(); it++){
		string name = it->first;
		//Transformations
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 rotateRectangle = glm::rotate((float)(gunparts[name].angle * M_PI/180.0f), glm::vec3(0,0,1));
		glm::mat4 translateRectangle = glm::translate (glm::vec3(-375.0 + gunparts[name].x, gunparts[name].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);

		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(gunparts[name].objectvao);
	}
	if(totalscore == 0) drawDig(0, 0);
	else {
		int score = totalscore;
		int place = 0;
		while(score){
			int dig = score % 10;
			drawDig(dig, place);
			score /= 10;
			place ++;
		}
	}
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 rotateRectangle = glm::rotate((float)(45.0 * M_PI/180.0f), glm::vec3(0,0,1));
	glm::mat4 translateRectangle = glm::translate (glm::vec3(lives["life1"].x, lives["life1"].y, 0));        // glTranslatef
	Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(lives["life1"].objectvao);
	if(life >= 2){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(lives["life2"].x, lives["life2"].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(lives["life2"].objectvao);

	}
	if(life == 3){
		Matrices.model = glm::mat4(1.0f);
		glm::mat4 translateRectangle = glm::translate (glm::vec3(lives["life3"].x, lives["life3"].y, 0));        // glTranslatef
		Matrices.model *= (translateRectangle * rotateRectangle /** scaleRectangle*/);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		draw3DObject(lives["life3"].objectvao);

	}
	// Increment angles
	//float increments = 1;

	//camera_rotation_angle++; // Simulating camera rotation
	//triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
	//rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
		GLFWwindow* window; // window desciptor/handle

		glfwSetErrorCallback(error_callback);
		if (!glfwInit()) {
//        exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		window = glfwCreateWindow(width, height, "Brick Breaker", NULL, NULL);

		if (!window) {
			glfwTerminate();
//        exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(window);
		gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
		glfwSwapInterval( 1 );

		/* --- register callbacks with GLFW --- */

		/* Register function to handle window resizes */
		/* With Retina display on Mac OS X GLFW's FramebufferSize
		 is different from WindowSize */
		glfwSetFramebufferSizeCallback(window, reshapeWindow);
		glfwSetWindowSizeCallback(window, reshapeWindow);

		/* Register function to handle window close */
		glfwSetWindowCloseCallback(window, quit);

		/* Register function to handle keyboard input */
		glfwSetKeyCallback(window, keyboard);      // general keyboard input
		glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

		/* Register function to handle mouse click */
		glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
		glfwSetScrollCallback(window, mousescroll);
		return window;
	}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
	void initGL (GLFWwindow* window, int width, int height)
	{
		/* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
		//Buckets
		createRectangle ("redBucket", red, 0.0, 200.0, -275.0, 50.0, 100.0, "buckets");
		createRectangle ("blueBucket", blue, 0.0, -200.0, -275.0, 50.0, 100.0, "buckets");
		//Cannons
		createRectangle ("gunBase1", black, 0.0, 0, 0, 75.0, 75.0, "gunparts");
		createRectangle ("gunBase2", black, 45.0, 0, 0, 75.0, 75.0, "gunparts");
		createRectangle ("gunArm", black, 0.0, 0, 0, 10.0, 150.0, "gunparts");
		//Mirrors
		createRectangle ("mirror1", grey, 135.0, 0, 135.0, 3.0, 50.0, "mirrors");
		createRectangle ("mirror1back", black, 135.0, 2.0, 137.0, 2.0, 50.0, "mirrors");
		createRectangle ("mirror2", grey, 45.0, 165.0, 70.0, 3.0, 50.0, "mirrors");
		createRectangle ("mirror2back", black, 45.0, 167.0, 68.0, 2.0, 50.0, "mirrors");
		createRectangle ("mirror3", grey, 45.0, 165.0, -135.0, 3.0, 50.0, "mirrors");
		createRectangle ("mirror3back", black, 45.0, 167.0, -137.0, 2.0, 50.0, "mirrors");
		createRectangle ("mirror4", grey, 315.0, -110.0, -25.0, 3.0, 50.0, "mirrors");
		createRectangle ("mirror4back", black, 315.0, -112.0, -27.0, 2.0, 50.0, "mirrors");
		//Display units
		createRectangle ("topright", black, 0.0, 7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("topleft", black, 0.0, -7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("top", black, 0.0, 0, 15.0, 5.0, 20.0, "display");
		createRectangle ("middle", black, 0.0, 0, 0, 5.0, 20.0, "display");
		createRectangle ("botright", black, 0.0, 7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("botleft", black, 0.0, -7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("bottom", black, 0.0, 0, -15.0, 5.0, 20.0, "display");
		//Display tens
		createRectangle ("topright1", black, 0.0, 7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("topleft1", black, 0.0, -7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("top1", black, 0.0, 0, 15.0, 5.0, 20.0, "display");
		createRectangle ("middle1", black, 0.0, 0, 0, 5.0, 20.0, "display");
		createRectangle ("botright1", black, 0.0, 7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("botleft1", black, 0.0, -7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("bottom1", black, 0.0, 0, -15.0, 5.0, 20.0, "display");
		//Display hundreds
		createRectangle ("topright2", black, 0.0, 7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("topleft2", black, 0.0, -7.5, 7.5, 20.0, 5.0, "display");
		createRectangle ("top2", black, 0.0, 0, 15.0, 5.0, 20.0, "display");
		createRectangle ("middle2", black, 0.0, 0, 0, 5.0, 20.0, "display");
		createRectangle ("botright2", black, 0.0, 7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("botleft2", black, 0.0, -7.5, -7.5, 20.0, 5.0, "display");
		createRectangle ("bottom2", black, 0.0, 0, -15.0, 5.0, 20.0, "display");
		//Lives
		createRectangle ("life1", green, 45.0, 310.0, 200.0, 10.0, 10.0, "lives");
		createRectangle ("life2", green, 45.0, 340.0, 200.0, 10.0, 10.0, "lives");
		createRectangle ("life3", green, 45.0, 370.0, 200.0, 10.0, 10.0, "lives");
		//Highlights
		createRectangle ("redBucketLight", yellow, 0.0, 200.0, -275.0, 55.0, 105.0, "highlights");
		createRectangle ("blueBucketLight", yellow, 0.0, -200.0, -275.0, 55.0, 105.0, "highlights");


	// Create and compile our GLSL program from the shaders
		programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
		Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

		reshapeWindow (window, width, height);

		// Background color of the scene
	glClearColor (1.0f, 1.0f, 1.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}
int main (int argc, char** argv)
{
	int width = 800;
	int height = 600;

	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;
	last_beam_time = glfwGetTime();
		/* Draw in loop */
	int ind = 0;
	char temp[10];
	brickspeed = 1.0;
	beamind = 0;
	totalscore = 0;
	life = 3;
	gameover = 0;
	zoomamount = 1.0;
	dx = 0;
	dy = 0;
	srand (time(NULL));
	while (!glfwWindowShouldClose(window)) {
		draw(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if(current_time - last_beam_time >= 1.0){
			gunparts["gunArm"].active = 0;
		}
		current_time = glfwGetTime();
		if ((current_time - last_update_time) >= (2.0 - brickspeed * 0.25)) { // atleast 0.5s elapsed since last frame
		  	float xcoord = rand() % 500 - 210;
		  	int newcolor = rand() % 10 + 1;
		  	snprintf(temp, 10, "%d", ind);
			string str (temp);
		  	if(newcolor <= 4)
				createRectangle ("brick" + str, red, 0.0, xcoord, 310.0, 20.0, 10.0, "bricks");
			else if(newcolor <= 8)
				createRectangle ("brick" + str, blue, 0.0, xcoord, 310.0, 20.0, 10.0, "bricks");
			else 
				createRectangle ("brick" + str, black, 0.0, xcoord, 310.0, 20.0, 10.0, "bricks");
			last_update_time = current_time;
			ind ++;
		}
	}
	cout << totalscore << endl;
	glfwTerminate();
//    exit(EXIT_SUCCESS);
}