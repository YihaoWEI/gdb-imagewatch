/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2017 GDB ImageWatch contributors
 * (github.com/csantosbh/gdb-imagewatch/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <GL/glew.h>
#include <cstring>

#include "shader.h"


ShaderProgram::ShaderProgram()
    : program_(0)
{
}


ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(program_);
}


bool ShaderProgram::is_shader_outdated(TexelChannels texel_format,
                                       const std::vector<std::string>& uniforms,
                                       const char* pixel_layout)
{
    // If the texel format or the uniform container size changed,
    // the program must be created again
    if (texel_format != texel_format_ || uniforms.size() != uniforms_.size()) {
        return true;
    }

    // The program must also be created again if an uniform name
    // changed
    for (const auto& uniform_name : uniforms) {
        if (uniforms_.find(uniform_name) == uniforms_.end()) {
            return true;
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (pixel_layout[i] != pixel_layout_[i]) {
            return true;
        }
    }

    // Otherwise, it must not change
    return false;
}

bool ShaderProgram::create(const char* v_source,
                           const char* f_source,
                           TexelChannels texel_format,
                           const char* pixel_layout,
                           const std::vector<std::string>& uniforms)
{
    if (program_ != 0) {
        // Check if the program needs to be recompiled
        if (!is_shader_outdated(texel_format, uniforms, pixel_layout)) {
            return true;
        }
        // Delete old program
        glDeleteProgram(program_);
    }

    texel_format_ = texel_format;
    memcpy(pixel_layout_, pixel_layout, 4);
    pixel_layout_[4]       = '\0';
    GLuint vertex_shader   = compile(GL_VERTEX_SHADER, v_source);
    GLuint fragment_shader = compile(GL_FRAGMENT_SHADER, f_source);

    if (vertex_shader == 0 || fragment_shader == 0) {
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertex_shader);
    glAttachShader(program_, fragment_shader);
    glLinkProgram(program_);

    // Delete shaders. We don't need them anymore.
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Get uniform locations
    for (const auto& name : uniforms) {
        GLuint loc      = glGetUniformLocation(program_, name.c_str());
        uniforms_[name] = loc;
    }

    return true;
}


void ShaderProgram::uniform1i(const std::string& name, int value)
{
    glUniform1i(uniforms_[name], value);
}


void ShaderProgram::uniform2f(const std::string& name, float x, float y)
{
    glUniform2f(uniforms_[name], x, y);
}


void ShaderProgram::uniform3fv(const std::string& name,
                               int count,
                               const float* data)
{
    glUniform3fv(uniforms_[name], count, data);
}


void ShaderProgram::uniform4fv(const std::string& name,
                               int count,
                               const float* data)
{
    glUniform4fv(uniforms_[name], count, data);
}


void ShaderProgram::uniform_matrix4fv(const std::string& name,
                                      int count,
                                      GLboolean transpose,
                                      const float* value)
{
    glUniformMatrix4fv(uniforms_[name], count, transpose, value);
}


void ShaderProgram::use()
{
    glUseProgram(program_);
}


GLuint ShaderProgram::compile(GLuint type, GLchar const* source)
{
    GLuint shader = glCreateShader(type);

    const char* src[] = {
        "#version 120\n",

        // clang-format off
        texel_format_ == FormatR ?   "#define FORMAT_R\n" :
        texel_format_ == FormatRG ?  "#define FORMAT_RG\n" :
        texel_format_ == FormatRGB ? "#define FORMAT_RGB\n" :
                                     "",
        // clang-format on

        "#define PIXEL_LAYOUT ",
        pixel_layout_,

        source};

    glShaderSource(shader, 5, src, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        std::cerr << "Failed to compile shadertype: " + get_shader_type(type)
                  << std::endl
                  << log << std::endl;
        return false;
    }
    return shader;
}


std::string ShaderProgram::get_shader_type(GLuint type)
{
    std::string name;
    switch (type) {
    case GL_VERTEX_SHADER:
        name = "Vertex Shader";
        break;
    case GL_FRAGMENT_SHADER:
        name = "Fragment Shader";
        break;
    default:
        name = "Unknown Shader type";
        break;
    }
    return name;
}
