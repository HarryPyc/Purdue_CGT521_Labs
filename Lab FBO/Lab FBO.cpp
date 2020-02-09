#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"

static const std::string vertex_shader("fbo_vs.glsl");
static const std::string fragment_shader("fbo_fs.glsl");

GLuint shader_program = -1;
GLuint texture_id = -1; //Texture map for fish

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

GLuint fbo_id = -1;       // Framebuffer object,
GLuint rbo_id = -1;       // and Renderbuffer (for depth buffering)
GLuint fbo_texture = -1;  // Texture rendered into.

GLuint depth_buffer = -1;


GLuint fbo_texture2 = -1;
GLuint plain_tex = -1;

static const std::string mesh_name = "bunny.obj";
static const std::string texture_name = "AmagoT.bmp";
MeshData mesh_data;
float time_sec = 0.0f;
const int width = 1280, height = 720;
glm::vec3 light(1.0f, 1.0f, 1.0f);

bool check_framebuffer_status();
glm::mat4 P, V, M, PVM;

void draw_pass_1()
{
   const int pass = 1;

   
   V = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   P = glm::perspective(60.0f, float(width)/height, 0.1f, 100.0f);

   int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }

   int M_loc = glGetUniformLocation(shader_program, "M");
   if (M_loc != -1) {
	   glUniformMatrix4fv(M_loc, 1, false, &M[0][0]);
   }

   int PVM_loc = glGetUniformLocation(shader_program, "PVM");
   if(PVM_loc != -1)
   {
      PVM = P*V*M;
      glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }
   
   int light_loc = glGetUniformLocation(shader_program, "light");
   if (light_loc != -1) {
	   glUniform3fv(light_loc, 1, &light[0]);
   }


   
   

   glBindVertexArray(mesh_data.mVao);
	glDrawElements(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0);

}

void draw_pass_2()
{
	const int pass = 2;
	int pass_loc = glGetUniformLocation(shader_program, "pass");
	if (pass_loc != -1)
	{
		glUniform1i(pass_loc, pass);
	}
	V = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	P = glm::ortho(-1.0f, 1.f, 1.f, -1.0f, 0.0f, 10.f);
	int PVM_loc = glGetUniformLocation(shader_program, "PVM");
	if (PVM_loc != -1)
	{
		PVM = P * V*M;
		glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	}
	glBindVertexArray(mesh_data.mVao);
	glDrawElements(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0);

}
void draw_pass_3() {
	const int pass = 3;
	int pass_loc = glGetUniformLocation(shader_program, "pass");
	if (pass_loc != -1)
	{
		glUniform1i(pass_loc, pass);
	}
	M = glm::rotate(-90.f, glm::vec3(1.0f, 0.0f, 0.0f));
	V = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	P = glm::perspective(60.0f, float(width) / height, 0.1f, 100.0f);
	int PVM_loc = glGetUniformLocation(shader_program, "PVM");
	if (PVM_loc != -1)
	{
		PVM = P * V*M;
		glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plain_tex);

	int tex_loc = glGetUniformLocation(shader_program, "texture");
	if (tex_loc != -1)
	{
		glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
	}

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
void draw_pass_4()
{
   const int pass = 4;
   int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, fbo_texture2);

   int tex_loc = glGetUniformLocation(shader_program, "texture");
   if(tex_loc != -1)
   {
      glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
   }
   int tex2_loc = glGetUniformLocation(shader_program, "texture2");
   if (tex2_loc != -1) {
	   glUniform1i(tex2_loc, 1);
   }

   glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{

   glUseProgram(shader_program);

   M = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(10.0f*time_sec, glm::vec3(0.0f, 1.0f, 0.0f))
	   * glm::scale(glm::vec3(mesh_data.mScaleFactor));
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); // Render to FBO.
   glDrawBuffer(GL_COLOR_ATTACHMENT0); //Out variable in frag shader will be written to the texture attached to GL_COLOR_ATTACHMENT0.

   //Make the viewport match the FBO texture size.
   int tex_w, tex_h;
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);
   glViewport(0, 0, tex_w, tex_h);

   //Clear the FBO.
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); //Lab assignment: don't forget to also clear depth
   glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
   draw_pass_1();

   glDrawBuffer(GL_COLOR_ATTACHMENT1);
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
   glClearColor(0.35f, 0.35f, 0.35f, 0.0f);
   draw_pass_2();
         
   glDrawBuffer(GL_COLOR_ATTACHMENT2);
   glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

   draw_pass_3();

   glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
   glDrawBuffer(GL_BACK); // Render to back buffer.

   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   glViewport(0, 0, w, h); //Render to the full viewport
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer

   draw_pass_4();
   
   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;
}

void reload_shader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if(new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

      if(shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;

      if(mesh_data.mVao != -1)
      {
         BufferIndexedVerts(mesh_data);
      }
   }
}


// glut keyboard callback function.
// This function gets called when an ASCII key is pressed
void keyboard(unsigned char key, int x, int y)
{
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

   switch(key)
   {
      case 'r':
      case 'R':
         reload_shader();     
      break;
   }
}

void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}


void initOpenGl()
{
   glewInit();

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_ALPHA_TEST);
   reload_shader();

   //mesh and texture for pass 1
   mesh_data = LoadMesh(mesh_name);
   //texture_id = LoadTexture(texture_name.c_str());

   //mesh for pass 2 (full screen quadrilateral)
   glGenVertexArrays(1, &quad_vao);
   glBindVertexArray(quad_vao);

   float vertices[] = {1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f};

   //create vertex buffers for vertex coords
   glGenBuffers(1, &quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   int pos_loc = glGetAttribLocation(shader_program, "pos_attrib");
   if(pos_loc >= 0)
   {
      glEnableVertexAttribArray(pos_loc);
	  glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
   }

  

   //Create texture to render pass 1 into.
   //Lab assignment: make the texture width and height be the window width and height.
   glGenTextures(1, &fbo_texture);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glBindTexture(GL_TEXTURE_2D, 0);   

   glGenTextures(1, &plain_tex);
   glBindTexture(GL_TEXTURE_2D, plain_tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenTextures(1, &fbo_texture2);
   glBindTexture(GL_TEXTURE_2D, fbo_texture2);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);
   //Lab assignment: Create renderbuffer for depth.
   glGenRenderbuffers(1, &depth_buffer);
   glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);


   //Create the framebuffer object
   glGenFramebuffers(1, &fbo_id);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, plain_tex, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fbo_texture2, 0);

   //Lab assignment: attach depth renderbuffer to FBO
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

   check_framebuffer_status();

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

int main (int argc, char **argv)
{
   //Configure initial window state
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (width, height);
   int win = glutCreateWindow ("Lab FBO solution");

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);

   initOpenGl();

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}

bool check_framebuffer_status() 
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			printf("Framebuffer incomplete, incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("Framebuffer incomplete, missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			printf("Framebuffer incomplete, missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("Framebuffer incomplete, missing read buffer\n");
            return false;
    }
	return false;
}


