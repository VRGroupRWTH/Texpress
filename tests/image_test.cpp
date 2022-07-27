#include <catch2/catch_all.hpp>
#include <texpress/api.hpp>

#include <iostream>
#include <compressonator.h>

#include <ktx.h>
#include <stdio.h>

struct rendering_pass : texpress::render_pass
{
  rendering_pass(texpress::Encoder* anEncoder)
    : encoder(anEncoder)
    , positions{ }
    , colors{ }
    , indices{ }
    , coordinates{ }
    , image_in{ }
    , imagef_in{ }
    , image_out{ }
    , vertex_buffer(globjects::Buffer::create())
    , color_buffer(globjects::Buffer::create())
    , coordinates_buffer(globjects::Buffer::create())
    , index_buffer(globjects::Buffer::create())
    , vertex_array(globjects::VertexArray::create())
    , texture_in(globjects::Texture::create())
    , texture_out(globjects::Texture::create())
    , vertex_shader_source(globjects::Shader::sourceFromString(vertex_shader_source_text))
    , fragment_shader_source(globjects::Shader::sourceFromString(fragment_shader_source_text))
    , vertex_shader_template(globjects::Shader::applyGlobalReplacements(vertex_shader_source.get()))
    , fragment_shader_template(globjects::Shader::applyGlobalReplacements(fragment_shader_source.get()))
    , vertex_shader(globjects::Shader::create(gl::GL_VERTEX_SHADER, vertex_shader_template.get()))
    , fragment_shader(globjects::Shader::create(gl::GL_FRAGMENT_SHADER, fragment_shader_template.get()))
    , program(globjects::Program::create())
  {
    on_prepare = [&]()
    {
      // Setup Configuratin
      // ==================
      program->attach(vertex_shader.get());
      program->attach(fragment_shader.get());
      program->link();

      texture_in->setParameter(gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
      texture_in->setParameter(gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
      texture_in->setParameter(gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_BORDER);
      texture_in->setParameter(gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_BORDER);

      texture_out->setParameter(gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
      texture_out->setParameter(gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
      texture_out->setParameter(gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_BORDER);
      texture_out->setParameter(gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_BORDER);

      // Setup Data
      // ==========

      positions.resize(4);
      colors.resize(4);
      coordinates.resize(4);
      indices.resize(6);

      positions[0] = { -1, -1, 0, 1 };
      positions[1] = {  1, -1, 0, 1 };
      positions[2] = {  1,  1, 0, 1 };
      positions[3] = { -1,  1, 0, 1 };

      colors[0] = { 1, 0, 0, 1 };
      colors[1] = { 1, 0, 0, 1 };
      colors[2] = { 1, 0, 0, 1 };
      colors[3] = { 0, 0, 0, 1 };

      coordinates[0] = { 0, 0 };
      coordinates[1] = { 1, 0 };
      coordinates[2] = { 1, 1 };
      coordinates[3] = { 0, 1 };

      indices[0] = 0;
      indices[1] = 1;
      indices[2] = 2;

      indices[3] = 2;
      indices[4] = 3;
      indices[5] = 0;

      if (bc6h) {
        imagef_in = texpress::load_image_hdr("../files/vr.jpg", 4);

        image_out = texpress::load_ktx<uint8_t>("3d_ktx_bc6h_neg.ktx2");
        image_out.gl_internalFormat = gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;

        //image_out = encoder->compress_bc6h(texpress::BC6H_options(), imagef_in);
       }
      else {
        image_in = texpress::load_image("../files/vr.jpg", 4);

        //image_out = encoder->compress_bc7(image_in);
      }

    };
    on_update = [&]()
    {
      vertex_array->bind();

      vertex_buffer->bind(gl::GLenum::GL_ARRAY_BUFFER);
      vertex_buffer->setData(positions, gl::GLenum::GL_STATIC_DRAW);
      gl::glVertexAttribPointer(0, 4, gl::GL_FLOAT, gl::GL_FALSE, 0, nullptr);
      gl::glEnableVertexAttribArray(0);

      color_buffer->bind(gl::GLenum::GL_ARRAY_BUFFER);
      color_buffer->setData(colors, gl::GLenum::GL_STATIC_DRAW);
      gl::glVertexAttribPointer(1, 4, gl::GL_FLOAT, gl::GL_FALSE, 0, nullptr);
      gl::glEnableVertexAttribArray(1);

      coordinates_buffer->bind(gl::GLenum::GL_ARRAY_BUFFER);
      coordinates_buffer->setData(coordinates, gl::GLenum::GL_STATIC_DRAW);
      gl::glVertexAttribPointer(2, 2, gl::GL_FLOAT, gl::GL_FALSE, 0, nullptr);
      gl::glEnableVertexAttribArray(2);

      if (GetKeyState('T') & 0x8000)
        show_tex_1 = !show_tex_1;

      if (show_tex_1)
      {
        texture_in->bindActive(0);
        if (bc6h) {
          texture_in->image2D(0, gl::GL_RGBA32F, imagef_in.size, 0, gl::GL_RGBA, gl::GL_FLOAT, imagef_in.data.data());
        }
        else {
          texture_in->image2D(0, gl::GL_RGBA8, image_in.size, 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, image_in.data.data());
        }
      }
      else
      {
        texture_out->bindActive(0);
        texture_out->compressedImage2D(0, image_out.gl_internalFormat, glm::ivec2(image_out.grid_size), 0, image_out.data_size, image_out.data.data());
      }

      index_buffer->bind(gl::GLenum::GL_ELEMENT_ARRAY_BUFFER);
      index_buffer->setData(indices, gl::GLenum::GL_STATIC_DRAW);

      program->use();
      vertex_array->bind();
      gl::glDrawElements(gl::GL_TRIANGLES, indices.size(), gl::GL_UNSIGNED_INT, nullptr);
    };
  }

  std::string vertex_shader_source_text = R"(#version 450
layout(location = 0) in  vec4 vert_position;
layout(location = 1) in  vec4 vert_color   ;
layout(location = 2) in  vec2 vert_uv;

layout(location = 0) out vec4 frag_color   ;
layout(location = 1) out vec2 tex_uv;

void main()
{
  frag_color  = vert_color;
  tex_uv      = vert_uv;
  gl_Position = vert_position;
}
)";
  std::string fragment_shader_source_text = R"(#version 450
layout(location = 0) in  vec4 frag_color ;
layout(location = 1) in  vec2 tex_uv;
uniform sampler2D tex;

layout(location = 0) out vec4 final_color;


void main()
{
  vec4 color = vec4(0, 0, 0, 1);
  color = frag_color;
  color = texture(tex, tex_uv.st);
  final_color = color;
}
)";

  bool bc6h = true;
  bool show_tex_1 = true;

  texpress::image_ldr image_in;
  texpress::image_hdr imagef_in;
  texpress::Texture<uint8_t> image_out;

  texpress::Encoder* encoder;

  std::vector<glm::vec4> positions;
  std::vector<glm::vec4> colors;
  std::vector<glm::vec2> coordinates;
  std::vector<std::uint32_t> indices;

  std::unique_ptr<globjects::Buffer>               vertex_buffer;
  std::unique_ptr<globjects::Buffer>               color_buffer;
  std::unique_ptr<globjects::Buffer>               coordinates_buffer;
  std::unique_ptr<globjects::Buffer>               index_buffer;
  std::unique_ptr<globjects::VertexArray>          vertex_array;

  std::unique_ptr<globjects::Texture>              texture_in;
  std::unique_ptr<globjects::Texture>              texture_out;

  std::unique_ptr<globjects::StaticStringSource>   vertex_shader_source;
  std::unique_ptr<globjects::StaticStringSource>   fragment_shader_source;
  std::unique_ptr<globjects::AbstractStringSource> vertex_shader_template;
  std::unique_ptr<globjects::AbstractStringSource> fragment_shader_template;
  std::unique_ptr<globjects::Shader>               vertex_shader;
  std::unique_ptr<globjects::Shader>               fragment_shader;

  std::unique_ptr<globjects::Program>              program;
};

TEST_CASE("Image test.", "[texpress::cli]")
{
  auto application = std::make_unique<texpress::application>();
  auto renderer = application->add_system<texpress::renderer>();
  auto dispatcher = application->add_system<texpress::Dispatcher>();
  auto encoder = application->add_system<texpress::Encoder>();
  ImGui::SetCurrentContext(application->gui());

  renderer->add_render_pass<texpress::render_pass>(texpress::make_clear_pass(application->window()));
  renderer->add_render_pass<rendering_pass>(encoder);
  renderer->add_render_pass<texpress::render_pass>(texpress::make_swap_pass(application->window()));

  application->run();

  ImGui::DestroyContext();
}