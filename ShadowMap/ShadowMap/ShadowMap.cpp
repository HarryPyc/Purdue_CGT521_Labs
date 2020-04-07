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

#include "imgui_impl_glut.h"
#include "InitShader.h"
#include "LoadMesh.h"

static const std::string vertex_shader("shadowmap_vs.glsl");
static const std::string fragment_shader("shadowmap_fs.glsl");
GLuint shader_program = -1;

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

GLuint fbo_id = -1;       // framebuffer object,
GLuint shadow_map_texture_id = -1;

int shadow_map_size = 1024; //Lab: this is the size of the shadow map. Try making it bigger.

int win_width = 1024;
int win_height = 1024;

static const std::string mesh_name = "Amago0.obj";
MeshData mesh_data;

//V_cam: position and orientation of camera
glm::mat4 V_cam = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//P_cam: field-of-view and near/far clip plane of camera
glm::mat4 P_cam = glm::perspective(40.0f, 1.0f, 0.1f, 10.0f);

//V_light: position and orientation of light
glm::mat4 V_light = glm::lookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//P_light: field-of-view and near/far clip plane of light
glm::mat4 P_light = glm::perspective(40.0f, 1.0f, 0.1f, 10.0f);

glm::mat4 M_fish;
glm::mat4 M_quad = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f))*glm::rotate(-45.0f, glm::vec3(1.0f, 0.0f, 0.0f))*glm::scale(glm::vec3(0.75f));

int render_mode = 1;
glm::vec2 offset(3.0);
bool PCF = true;
bool displayLit4 = false;
float softness = 1.0f;
bool check_framebuffer_status();
void draw_gui() {
    ImGui_ImplGlut_NewFrame();
    
    ImGui::Begin("Console");
    ImGui::SliderFloat2("PolygonOffset", &offset[0], 0.0f, 10.0f);
    if (PCF) ImGui::Text("2x2 PCF");
    else ImGui::Text("4x4 PCF");
    if (ImGui::Button("ChangePCF")) {
        PCF = !PCF;
        glUniform1f(glGetUniformLocation(shader_program, "PCF"), PCF);
    }
    if (ImGui::SliderFloat("Softness", &softness, 0.0f, 5.0f)) {
        glUniform1f(glGetUniformLocation(shader_program, "softness"), softness);
    }
    if (ImGui::Checkbox("Display lit4", &displayLit4)) {
        glUniform1f(glGetUniformLocation(shader_program, "displayLit4"), displayLit4);
    }
    ImGui::End();

    ImGui::Render();
}
//draw the scene using the specified view matrix
void draw_scene(glm::mat4& V)
{
   //draw mesh
   int VM_loc = glGetUniformLocation(shader_program, "VM");
   if(VM_loc != -1)
   {
      const glm::mat4 VM = V*M_fish;
      glUniformMatrix4fv(VM_loc, 1, false, glm::value_ptr(VM));
   }
   glBindVertexArray(mesh_data.mVao);
	glDrawElements(GL_TRIANGLES, mesh_data.mNumIndices, GL_UNSIGNED_INT, 0);


   //draw plane
   if(VM_loc != -1)
   {
      const glm::mat4 VM = V*M_quad;
      glUniformMatrix4fv(VM_loc, 1, false, glm::value_ptr(VM));
   }
   glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_pass_1() //draw from light point-of-view
{
   const int pass = 1;

   const int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }

   const int P_loc = glGetUniformLocation(shader_program, "P");
   if(P_loc != -1)
   {
      glUniformMatrix4fv(P_loc, 1, false, glm::value_ptr(P_light));
   }

   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(offset[0], offset[1]); //No offset is being applied when params are 0.0, 0.0. Try changing these numbers to fix the shadow map "acne" problem

   draw_scene(V_light);
   glDisable(GL_POLYGON_OFFSET_FILL);
}

void draw_pass_2()
{
   const int pass = 2;

   const int pass_loc = glGetUniformLocation(shader_program, "pass");
   if(pass_loc != -1)
   {
      glUniform1i(pass_loc, pass);
   }

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, shadow_map_texture_id);

   const int tex_loc = glGetUniformLocation(shader_program, "shadowmap");
   if(tex_loc != -1)
   {
      glUniform1i(tex_loc, 0); // we bound our shadowmap to texture unit 0
   }

   const int P_loc = glGetUniformLocation(shader_program, "P");
   if(P_loc != -1)
   {
      glUniformMatrix4fv(P_loc, 1, false, glm::value_ptr(P_cam));
   }

   const int Shadow_loc = glGetUniformLocation(shader_program, "Shadow");
   const glm::mat4 S = glm::translate(glm::vec3(0.5f)) * glm::scale(glm::vec3(0.5f));
   if(Shadow_loc != -1)
   {
      const glm::mat4 Shadow = S*P_light*V_light*glm::inverse(V_cam); //this matrix transforms camera-space coordinates to shadow map texture coordinates
      glUniformMatrix4fv(Shadow_loc, 1, false, glm::value_ptr(Shadow));
   }

   draw_scene(V_cam);
}


// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
   glUseProgram(shader_program);

   if(render_mode == 1) // draw scene with shadowmapping
   {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); // render depth to shadowmap
      glDrawBuffer(GL_NONE);
      glViewport(0, 0, shadow_map_size, shadow_map_size); 
      //glViewport(0, 0, shadow_map_size-8, shadow_map_size-8);  //*hack* (Why does this work?)
      glClear(GL_DEPTH_BUFFER_BIT);
      draw_pass_1(); //Pass 1: render depth to texture from light point of view
   
      glBindFramebuffer(GL_FRAMEBUFFER, 0); // do not render to texture
      glDrawBuffer(GL_BACK);
      glViewport(0, 0, win_width, win_height);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
      draw_pass_2(); //Pass 2: render scene with shadowing
   }
   else if (render_mode == 2) // show shadow map for debugging
   {
      glBindFramebuffer(GL_FRAMEBUFFER, 0); // do not render to texture, render to back buffer
      glDrawBuffer(GL_BACK);
      glViewport(0, 0, shadow_map_size, shadow_map_size);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
      draw_pass_1();
   }
   draw_gui();
   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   float time_sec = 0.001f*time_ms;

   M_fish = glm::rotate(15.0f*time_sec, glm::vec3(1.0f, 0.0f, 0.0f))*glm::scale(glm::vec3(mesh_data.mScaleFactor));
   
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
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;
   ImGui_ImplGlut_KeyCallback(key);
   switch(key)
   {
      case 'r':
      case 'R':
         reload_shader();     
      break;

      case '1':
      case '2':
         render_mode = key-'1'+1;     
      break;
   }
}
void mouse(int button, int state, int x, int y) {
    ImGui_ImplGlut_MouseButtonCallback(button, state);
}
void motion(int x, int y) {
    ImGui_ImplGlut_MouseMotionCallback(x, y);
}
void passive(int x, int y) {
    ImGui_ImplGlut_PassiveMouseMotionCallback(x, y);
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
   ImGui_ImplGlut_Init();
   std::cout<<"Press 1 to show scene\n";
   std::cout<<"Press 2 to show shadow map texture\n";
   std::cout<<"Press r to reload shader\n";

   glEnable(GL_DEPTH_TEST);

   reload_shader();

   mesh_data = LoadMesh(mesh_name);
   M_fish = glm::scale(glm::vec3(mesh_data.mScaleFactor));

   //mesh for quadrilateral
   quad_vbo = -1;

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

   //create shadow map texture to hold depth values (note: not a renderbuffer)
   glGenTextures(1, &shadow_map_texture_id);
   glBindTexture(GL_TEXTURE_2D, shadow_map_texture_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadow_map_size, shadow_map_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
   glBindTexture(GL_TEXTURE_2D, 0);   

   //create the framebuffer object: only has a depth attachment
   glGenFramebuffers(1, &fbo_id);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_texture_id, 0);
   check_framebuffer_status();
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


int main (int argc, char **argv)
{
   //Configure initial window state
   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (win_width, win_height);
   int win = glutCreateWindow ("Shadow Map");

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   
   glutKeyboardFunc(keyboard);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(passive);
   glutIdleFunc(idle);
   glutReshapeFunc(reshape);

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


