#include <windows.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "InitShader.h"
#include "imgui_impl_glut.h"
#include "VideoMux.h"
#include "DebugCallback.h"

#include "ShaderLocs.h"

//names of the shader files to load
static const std::string vertex_shader("raycast_vs.glsl");
static const std::string fragment_shader("raycast_fs.glsl");
GLuint shader_program = -1;

//VAO for attributeless rendering
GLuint attribless_vao = -1;

void draw_attribless_cube()
{
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
}

//Texture which we will render into
GLuint fbo_texture = -1;
GLuint fbo_texture_width = 1280;
GLuint fbo_texture_height = 720;

//The frame buffer object provides render-to-texture functionality 
GLuint fbo;

float time_sec = 0.0f;

//Camera params
float angle = 0.0f;
float fov = 3.141592f / 4.0f;
float height = 2.0f;

bool recording = false;

void display_inside_out();
void display_outside_in();

//Draw the user interface using ImGui
void draw_gui()
{
   ImGui_ImplGlut_NewFrame();

   const int filename_len = 64;
   static char video_filename[filename_len] = "capture.mp4";

   ImGui::InputText("Video filename", video_filename, filename_len);
   ImGui::SameLine();
   if (recording == false)
   {
      if (ImGui::Button("Start Recording"))
      {
         const int w = glutGet(GLUT_WINDOW_WIDTH);
         const int h = glutGet(GLUT_WINDOW_HEIGHT);
         recording = true;
         start_encoding(video_filename, w, h); //Uses ffmpeg
      }
   }
   else
   {
      if (ImGui::Button("Stop Recording"))
      {
         recording = false;
         finish_encoding(); //Uses ffmpeg
      }
   }

   ImGui::Columns(3);
      ImGui::Image((void*)fbo_texture, ImVec2(128.0f, 128.0f), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
      ImGui::NextColumn();

      static int mode = 0;
      ImGui::RadioButton("Show back faces", &mode, 0);
      ImGui::RadioButton("Show front faces", &mode, 1);
      ImGui::RadioButton("Show raycasting", &mode, 2);
      glUniform1i(UniformLoc::Mode, mode);

      static bool outside_in = true;
      if (ImGui::Checkbox("Outside in", &outside_in))
      {
         if (outside_in == true)
         {
            glutDisplayFunc(display_outside_in);
         }
         else
         {
            glutDisplayFunc(display_inside_out);
         }
      }

      ImGui::NextColumn();

      static int scene = 0;
      ImGui::RadioButton("Scene 0", &scene, 0);
      ImGui::RadioButton("Scene 1", &scene, 1);
      ImGui::RadioButton("Scene 2", &scene, 2);

      glUniform1i(UniformLoc::Scene, scene);

   ImGui::Columns(1);
   
   ImGui::SliderFloat("Height", &height, -10.0f, +10.0f);
   ImGui::SliderFloat("View angle", &angle, -3.141592f, +3.141592f);
   ImGui::SliderFloat("FOV", &fov, 0.0f, +3.141592f);

   static glm::vec4 slider(0.0f);
   if(ImGui::SliderFloat4("Slider", &slider[0], -1.0f, +1.0f))
   {
      glUniform4fv(UniformLoc::Slider, 1, &slider[0]);
   }

   ImGui::Render();
 }

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display_outside_in()
{
   glUseProgram(shader_program);

   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   const float aspect_ratio = float(w) / float(h);

   //Set up some uniform variables for transformation matrices
   glm::mat4 M = glm::scale(glm::vec3(1.0f));
   glm::mat4 V = glm::lookAt(glm::vec3(2.0f, 2.0f, height), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f))*glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
   glm::mat4 P = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
   glm::mat4 T(1.0f);

   //we are using layout qualifiers in the shader
   glm::mat4 PV = P*V;
   glUniformMatrix4fv(UniformLoc::PV, 1, false, glm::value_ptr(PV));
   glUniformMatrix4fv(UniformLoc::M, 1, false, glm::value_ptr(M));
   glUniformMatrix4fv(UniformLoc::T, 1, false, glm::value_ptr(T));

   ///////////////////////////////////////////////////
   // Begin pass 1: render proxy geometry back faces to texture.
   ///////////////////////////////////////////////////
   
   //Set pass uniform variable.
   glUniform1i(UniformLoc::Pass, 1);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
   glDrawBuffer(GL_COLOR_ATTACHMENT0); //Out variable in frag shader will be written to the texture attached to GL_COLOR_ATTACHMENT0.

   //Clear the FBO attached texture.
   glClear(GL_COLOR_BUFFER_BIT);

   glCullFace(GL_FRONT);   //Don't draw front faces
   draw_attribless_cube(); //Draw the cube

   ///////////////////////////////////////////////////
   // Begin pass 2: render proxy geometry back faces to screen
   ///////////////////////////////////////////////////
   glUniform1i(UniformLoc::Pass, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
   glDrawBuffer(GL_BACK); // Render to back buffer.

   //Bind texture so we can read the texture in the shader
   //(using explicit binding in layout qualifier, so no glUniform1i() needed)
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
 
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer

   glCullFace(GL_BACK);    // Don't draw back faces
   draw_attribless_cube(); // Draw the cube to the screen.
         
   draw_gui();

   if (recording == true)
   {
      glFinish();

      glReadBuffer(GL_BACK);
      read_frame_to_encode(&rgb, &pixels, w, h);
      encode_frame(rgb);
   }

   glutSwapBuffers();
}

void display_inside_out()
{
   glUseProgram(shader_program);

   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   const float aspect_ratio = float(w) / float(h);

   //Set up some uniform variables
   glm::mat4 M1 = glm::scale(glm::vec3(1.0f));
   glm::mat4 M2 = glm::scale(glm::vec3(10.0f));
   glm::mat4 V = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
   V[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
   glm::mat4 P = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
   glm::mat4 T = glm::translate(glm::vec3(2.0f, 2.0f, height));
   
   //we are using layout qualifiers in the shader
   glUniformMatrix4fv(UniformLoc::PV, 1, false, glm::value_ptr(P*V));
   glUniformMatrix4fv(UniformLoc::T, 1, false, glm::value_ptr(T));

   ///////////////////////////////////////////////////
   // Begin pass 1: render proxy geometry back faces to texture.
   ///////////////////////////////////////////////////
   //Set pass uniform variable.
   
   glUniformMatrix4fv(UniformLoc::M, 1, false, glm::value_ptr(M2));
   glUniform1i(UniformLoc::Pass, 1);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
   glDrawBuffer(GL_COLOR_ATTACHMENT0); //Out variable in frag shader will be written to the texture attached to GL_COLOR_ATTACHMENT0.

   //Clear the FBO attached texture.
   glClear(GL_COLOR_BUFFER_BIT);

   glCullFace(GL_FRONT);   // Don't draw front faces
   draw_attribless_cube(); // Draw the cube

   ///////////////////////////////////////////////////
   // Begin pass 2: render proxy geometry front faces to screen
   ///////////////////////////////////////////////////
   glUniformMatrix4fv(UniformLoc::M, 1, false, glm::value_ptr(M1));
   glUniform1i(UniformLoc::Pass, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
   glDrawBuffer(GL_BACK); // Render to back buffer.

   //Bind texture so we can read the texture in the shader
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer
   draw_attribless_cube(); //Draw the cube to the screen.

   glCullFace(GL_BACK);
   
   draw_gui();

   if (recording == true)
   {
      glFinish();

      glReadBuffer(GL_BACK);
      read_frame_to_encode(&rgb, &pixels, w, h);
      encode_frame(rgb);
   }

   glutSwapBuffers();
}


// glut idle callback.
//This function gets called between frames
void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;

   glUniform1f(UniformLoc::Time, time_sec);
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
      glClearColor(0.15f, 0.15f, 0.15f, 0.0f);

      if(shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;
   }
}

// Display info about the OpenGL implementation provided by the graphics driver.
// Your version should be > 4.0 for CGT 521 
void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void initOpenGl()
{
   //Initialize glew so that new OpenGL function names can be used
   glewInit();

   RegisterCallback();

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glFrontFace(GL_CW);

   reload_shader();


   //Load the quadrilateral into a vao/vbo
   glGenVertexArrays(1, &attribless_vao);
   glBindVertexArray(attribless_vao);
   

   //Create a texture object and set initial wrapping and filtering state.
   //For raycasting create a floating point texture so we have high precision storage for ray endpoints.
   glGenTextures(1, &fbo_texture);
   glBindTexture(GL_TEXTURE_2D, fbo_texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo_texture_width, fbo_texture_height, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);


   //Create the framebuffer object
   glGenFramebuffers(1, &fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   //attach the texture we just created to color attachment 1
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

   //unbind the fbo
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// glut callbacks need to send keyboard and mouse events to imgui
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
   //Configure initial window state using freeglut

#if _DEBUG
   glutInitContextFlags(GLUT_DEBUG);
#endif
   glutInitContextVersion(4, 3);

   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (fbo_texture_width, fbo_texture_height);
   int win = glutCreateWindow (argv[0]);

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display_outside_in);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(special);
   glutKeyboardUpFunc(keyboard_up);
   glutSpecialUpFunc(special_up);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(motion);

   glutIdleFunc(idle);

   initOpenGl();
   ImGui_ImplGlut_Init(); // initialize the imgui system

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}


