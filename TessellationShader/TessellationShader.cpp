#include <windows.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>

#include "imgui/imgui_impl_glut.h"

#include <iostream>
#include <string>

#include "InitShader.h"

#define BUFFER_OFFSET(i)    ((char*)NULL + (i))

//4 shaders, including tess. control and tess. evaluation
static const std::string vertex_shader("tessellation_vs.glsl");
static const std::string tess_control_shader("tessellation_tc.glsl");
static const std::string tess_eval_shader("tessellation_te.glsl");
static const std::string fragment_shader("tessellation_fs.glsl");
GLuint shader_program = -1;

//VAO and VBO for the tessellated patch
GLuint patch_vao = -1;
GLuint patch_vbo = -1;

int win_width = 800;
int win_height = 800;

void draw_gui()
{
   ImGui_ImplGlut_NewFrame();

   const int n_sliders = 6;
   static float slider[n_sliders] = { 1.0f,1.0f,1.0f,1.0f,1.0f,1.0f };
   const std::string labels[n_sliders] = {"gl_TessLevelOuter[0]","gl_TessLevelOuter[1]","gl_TessLevelOuter[2]",
       "gl_TessLevelOuter[3]","gl_TessLevelInner[0]","gl_TessLevelInner[1]" };
   for (int i = 0; i<n_sliders; i++)
   {
      ImGui::SliderFloat(labels[i].c_str(), &slider[i], 1, 32);
   }
   int slider_loc = glGetUniformLocation(shader_program, "slider");
   glUniform1fv(slider_loc, n_sliders, slider);

   ImGui::Render();
 }

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glUseProgram(shader_program);

   glBindVertexArray(patch_vao);
   glPatchParameteri(GL_PATCH_VERTICES, 4); //number of input verts to the tess. control shader per patch.

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //Draw wireframe so we can see the edges of generated triangles
	glDrawArrays(GL_PATCHES, 0, 4); //Draw patches since we are using a tessellation shader.
   
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   draw_gui();

   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   float time_sec = 0.001f*time_ms;
   
}

void reload_shader()
{
   //Use the version of InitShader with 4 parameters. The shader names are in the order the stage are in the pipeline:
   //Vertex shader, Tess. control, Tess. evaluation, fragment shader
   GLuint new_shader = InitShader(vertex_shader.c_str(), tess_control_shader.c_str(), tess_eval_shader.c_str(), fragment_shader.c_str());

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
   }
}


void reshape(int w, int h)
{
   win_width = w;
   win_height = h;
   glViewport(0, 0, w, h);
}

// glut keyboard callback function.
// This function gets called when an ASCII key is pressed
void keyboard(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyCallback(key);
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

   reload_shader();

   //mesh for quadrilateral
   patch_vbo = -1;

   glGenVertexArrays(1, &patch_vao);
   glBindVertexArray(patch_vao);

   float vertices[] = { -1.0f,  1.0f, 0.0f, 
                        -1.0f, -1.0f, 0.0f, 
                         1.0f,  1.0f, 0.0f,
                         1.0f, -1.0f, 0.0f };
   
   //create vertex buffers for vertex coords
   glGenBuffers(1, &patch_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, patch_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   int pos_loc = glGetAttribLocation(shader_program, "pos_attrib");
   if(pos_loc >= 0)
   {
      glEnableVertexAttribArray(pos_loc);
	   glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
   }
}

void keyboard_up(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyUpCallback(key);
}

void special_up(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialUpCallback(key);
}

void passive(int x, int y)
{
   ImGui_ImplGlut_PassiveMouseMotionCallback(x,y);
}

void special(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialCallback(key);
}

void motion(int x, int y)
{
   ImGui_ImplGlut_MouseMotionCallback(x, y);
}

void mouse(int button, int state, int x, int y)
{
   ImGui_ImplGlut_MouseButtonCallback(button, state);
}

int main (int argc, char **argv)
{
   //Configure initial window state
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (win_width, win_height);
   int win = glutCreateWindow ("Tessellation Shader");

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   glutSpecialFunc(special);
   glutKeyboardUpFunc(keyboard_up);
   glutSpecialUpFunc(special_up);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(motion);
   glutReshapeFunc(reshape);

   initOpenGl();
   ImGui_ImplGlut_Init(); // initialize the imgui system

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}



