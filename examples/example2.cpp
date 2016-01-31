// you have to define the xml library you want to use for the xml
// parser. STLL needs to be compiled with this library
#define USE_PUGI_XML
#include <stll/layouterXHTML.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
// include your OpenGL stuff _before_ this header
#include <stll/output_OpenGL.h>


using namespace STLL;

int main()
{
  // setup a stylesheet, this will automatically creata a font cache
  // for you. typically you will have one or only a few stylesheets in
  // your application, if you have more than one you should share the
  // font cache between them
  TextStyleSheet_c styleSheet;

  // add the fonts that are required
  // we create one font family named "sans" with 2 members, a normal and a bold one
  styleSheet.addFont("sans", FontResource_c("tests/FreeSans.ttf"));
  styleSheet.addFont("sans", FontResource_c("tests/FreeSansBold.ttf"), "normal", "normal", "bold");

  // add the CSS rules you need, this should be familiar to everybody knowing CSS
  styleSheet.addRule("body", "color", "#ffffff");
  styleSheet.addRule("body", "font-size", "20px");
  styleSheet.addRule("body", "text-align", "justify");
  styleSheet.addRule("body", "padding", "10px");
  styleSheet.addRule("h1", "font-weight", "bold");
  styleSheet.addRule("h1", "font-size", "60px");
  styleSheet.addRule("h1", "text-align", "center");
  styleSheet.addRule("h1", "background-color", "#FF8080");

  // The XHTML code we want to format, it needs to be utf-8 encoded
  std::string text = u8"<html><body><h1>Title</h1><p>Some text</p></body></html>";

  // layout the XHTML code, in a 200 pixel wide rectangle
  auto layout = layoutXHTMLPugi(text, styleSheet, RectangleShape_c(200*64));


 
  // output the layout using OpenGL... this is a bit more complicated than SDL
  // the example uses glfw for the context creation and glew for extensions

  // initialite glfw, setup window hints
  if (!glfwInit()) return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // this hint is important for sRGB correct output
  glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

  // create the context and activate it
  GLFWwindow* screen = glfwCreateWindow(800, 600, "OpenGL example", NULL, NULL);
  if(!screen) return 1;
  glfwMakeContextCurrent(screen);

  // glew needs to be initialized _after_ the context has been created, also
  // this strange experimental flag has to be active... otherwise it will not
  // work, at least for the current version
  glewExperimental = true;
  if (glewInit() != GLEW_OK) return 1;

  // create out output object using OpenGL 3 features, use the default values for
  // the texture cache... which is too big here, but should be good for normal usage
  showOpenGL<3> openGL;
  // this sets up the projection matrix, or the shader equivalents for them, you
  // need to call it only once for OpenGL3, except when you change the viewport
  // for OpenGL 1 you need to call it whenever you start outputting as it sets up
  // modelview and projection matricies
  openGL.setupMatrixes(800, 600);
  // this is the drawing cache, we don't need it for this example, as we only draw once
  // but to show how it is supposed to work...
  showOpenGL<3>::DrawCache_c dc;

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  // don't forget to actually enable the sRGB framebuffer...
  glEnable(GL_FRAMEBUFFER_SRGB);
  // this is the actual draw call for the layout, if you don't want a cache, simply
  // give a nullptr to the function instead of &dc
  // after this call the cache is "bound" to the layout. You may only use it with this
  // layout, everything else will not work
  openGL.showLayout(layout, 20*64, 20*64, SUBP_RGB, nullptr, &dc);
  glfwSwapBuffers(screen);
  // wait until user closed the window
  while (!glfwWindowShouldClose(screen)) glfwPollEvents();
  glfwDestroyWindow(screen);
  glfwTerminate();

  return 0;
}
