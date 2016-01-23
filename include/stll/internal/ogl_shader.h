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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef STLL_OGL_SHADER_H
#define STLL_OGL_SHADER_H

#include <string.h>

namespace STLL { namespace internal {

class OGL_Program_c
{
  private:

    GLuint handle;

  public:

    OGL_Program_c(void)
    {
      handle = glCreateProgram();
    }

    ~OGL_Program_c(void)
    {
      glDeleteProgram(handle);
    }

    void attachShader(GLenum type, const std::string & version, const std::string & source)
    {
      GLuint sh_handle = glCreateShader(type);
      std::string program = std::string("#version ") + version + "\n" + source;
      const char * ptr = program.c_str();
      glShaderSource(sh_handle, 1, &ptr, 0 );
      glCompileShader(sh_handle);

      GLint compile_status;
      glGetShaderiv(sh_handle, GL_COMPILE_STATUS, &compile_status);
      if (compile_status == GL_FALSE)
      {
        fprintf(stderr, "error compiling shader %i %i\n", type, GL_VERTEX_SHADER);

        GLint maxLength = 0;
        glGetShaderiv(sh_handle, GL_INFO_LOG_LENGTH, &maxLength);

        printf("%i\n", maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(sh_handle, maxLength, &maxLength, &infoLog[0]);

        for (size_t i = 0; i < infoLog.size(); i++)
          printf("%c", infoLog[i]);


        // TODO exception
        exit(1);
      }

      glAttachShader(handle, sh_handle);
    }

    void bindAttributeLocation(int position, const std::string & name)
    {
      glBindAttribLocation(handle, position, name.c_str());
    }

    void link(void)
    {
      glLinkProgram(handle);

      GLint status;
      glGetProgramiv(handle, GL_LINK_STATUS, &status);
      if (status == GL_FALSE)
      {
        fprintf(stderr, "error linking program");
        exit(1);  // TODO throw exception
      }
    }

    void use(void)
    {
      glUseProgram(handle);
    }

    void setUniform(const std::string & name, const float matrix[16])
    {
      auto l = glGetUniformLocation(handle, name.c_str());
      if (l == -1)
      {
        printf("uniform %s not found\n", name.c_str());
      }
      else
      {
        use();
        glUniformMatrix4fv(l, 1, GL_FALSE, matrix);
      }
    }

    void setUniform(const std::string & name, float val)
    {
      auto l = glGetUniformLocation(handle, name.c_str());
      if (l == -1)
      {
        printf("uniform %s not found\n", name.c_str());
      }
      else
      {
        use();
        glUniform1f(l, val);
      }
    }

    void setUniform(const std::string & name, int val)
    {
      auto l = glGetUniformLocation(handle, name.c_str());
      if (l == -1)
      {
        printf("uniform %s not found\n", name.c_str());
      }
      else
      {
        use();
        glUniform1i(l, val);
      }
    }
};

} }

#endif
