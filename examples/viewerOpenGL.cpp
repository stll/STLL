/*
 * STLL Simple Text Layouting Library
 *
 * STLL is the legal property of its developers, whose
 * names are listed in the COPYRIGHT file, which is included
 * within the source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stll/layouter.h>
#include <stll/layouterFont.h>
#include "layouterXMLSaveLoad.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stll/output_OpenGL.h>

#if defined (OGL1)
#define OGL_VER 1
#elif defined (OGL2)
#define OGL_VER 2
#elif defined (OGL3)
#define OGL_VER 3
#endif

using namespace STLL;

static void key_callback(GLFWwindow* window, int key, int, int action, int)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argv, char ** args)
{
  pugi::xml_document doc;

  if (argv != 2)
  {
    printf("specify the layout to load as an argument\n");
    return 1;
  }

  auto res = doc.load_file(args[1]);

  if (!res)
  {
    printf("%s\n", (std::string("oopsi loading file...") + res.description()).c_str());
    return 1;
  }

  auto c = std::make_shared<FontCache_c>();
  auto l = loadLayoutFromXML(doc.child("layout"), c);

  if (!glfwInit())
  {
    fprintf(stderr, "Failed to initialize GLFW");
    return 1;
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

#if defined (OGL1)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#elif defined (OGL2)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#elif defined (OGL3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow* screen = glfwCreateWindow(l.getRight()/64, l.getHeight()/64, "OpenGL Layout Viewer", NULL, NULL);

  if(!screen)
  {
    fprintf(stderr, "Failed to create OpenGL window");
    return 1;
  }

  glfwMakeContextCurrent(screen);
  glfwSetKeyCallback(screen, key_callback);

  glewExperimental = true;
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err) );
    return 1;
  }

  showOpenGL<OGL_VER> openGL;

  int width, height;
  glfwGetFramebufferSize(screen, &width, &height);
  openGL.setupMatrixes(width, height);

  int x = 0;
  double startSeconds = glfwGetTime();

  showOpenGL<OGL_VER>::DrawCache_c dc;

  while (!glfwWindowShouldClose(screen))
  {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(50.0/255, 50.0/255, 50.0/255);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    for (int x = 0; x < 1+(int)l.getRight()/640; x++)
      for (int y = 0; y < 1+(int)l.getHeight()/640; y++)
      {
        if ((x + y) % 2)
        {
          glVertex3f(0.5+10*x,    0.5+10*y,    0);
          glVertex3f(0.5+10*x+10, 0.5+10*y,    0);
          glVertex3f(0.5+10*x+10, 0.5+10*y+10, 0);
          glVertex3f(0.5+10*x+10, 0.5+10*y+10, 0);
          glVertex3f(0.5+10*x,    0.5+10*y+10, 0);
          glVertex3f(0.5+10*x,    0.5+10*y,    0);
        }
      }
    glEnd();

    glEnable(GL_FRAMEBUFFER_SRGB);
    // openGL.showLayout(l, x, 0, SUBP_RGB); uncached version, much slower
    openGL.showLayout(l, x, 0, SUBP_RGB, nullptr, &dc);
    x++;
    glDisable(GL_FRAMEBUFFER_SRGB);
    glfwSwapBuffers(screen);
    glfwPollEvents();

    // this number is limited by screen update times, if you want a proper
    // number comment out the swap
    printf("\r %f images per Second", 1.0*x/ (glfwGetTime()-startSeconds));
  }
#if 0
  {
    // write out the texture map, the data files can be load with gimp and you will need to specify
    // the right size and graylevel as parameters in the dialog that pops up
    FILE * f = fopen("tex.data", "wb");
    fwrite(openGL.getData(), 1, openGL.cacheWidth()*openGL.cacheHeight(), f);
    fclose(f);
  }
#endif
  glfwDestroyWindow(screen);
  glfwTerminate();

}
