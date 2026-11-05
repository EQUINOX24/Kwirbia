#include "renderer.hpp"
#include "displaygrid.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* VERTEX_SHADER_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

uniform vec2 u_viewport;

out vec2 v_uv;
out vec4 v_color;

void main() {
    // Pixel coordinates (origin top-left) -> normalized device coordinates
    vec2 ndc = a_pos / u_viewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}
)glsl";

constexpr const char* FRAGMENT_SHADER_SRC = R"glsl(
#version 330 core
in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_atlas;

out vec4 frag_color;

void main() {
    // Atlas stores glyph coverage only (white-on-transparent PNG -> single channel)
    float coverage = v_uv.x < 0.0 ? 1.0 : texture(u_atlas, v_uv).r;
    frag_color = vec4(v_color.rgb, v_color.a * coverage);
}
)glsl";

GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error: ") + log);
    }
    return shader;
}

GLuint link_program(const char* vs_source, const char* fs_source)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_source);
    GLuint fs = 0;
    try {
        fs = compile_shader(GL_FRAGMENT_SHADER, fs_source);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Shader link error: ") + log);
    }
    return program;
}

} // namespace

Renderer::Renderer(std::span<const int32_t> font_sizes)
{
    program_ = link_program(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
    u_viewport_ = glGetUniformLocation(program_, "u_viewport");
    u_atlas_ = glGetUniformLocation(program_, "u_atlas");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_); // Element buffer binding is stored in the VAO

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, color)));
    glBindVertexArray(0);

    load_font_atlases(font_sizes);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer()
{
    for (GLuint texture : font_textures_) {
        if (texture) glDeleteTextures(1, &texture);
    }
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (program_) glDeleteProgram(program_);
}

void Renderer::load_font_atlases(std::span<const int32_t> font_sizes)
{
    font_sizes_.assign(font_sizes.begin(), font_sizes.end());
    font_textures_.assign(font_sizes.size(), 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (std::size_t i = 0; i < font_sizes.size(); ++i) {
        const int32_t size = font_sizes[i];
        const std::string name = "BlockyBold_" + std::to_string(size) + "x" + std::to_string(size) + ".png";

        SDL_Surface* loaded = SDL_LoadPNG(("media/" + name).c_str());
        if (!loaded) {
            loaded = SDL_LoadPNG(("src/media/" + name).c_str());
        }
        if (!loaded) {
            std::cerr << "Warning: Could not load font image for size " << size << ": " << SDL_GetError() << "\n";
            continue;
        }

        // Normalize to byte-ordered RGBA so the alpha channel is always byte 3
        SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(loaded);
        if (!rgba) {
            std::cerr << "Warning: Could not convert font image for size " << size << ": " << SDL_GetError() << "\n";
            continue;
        }

        // Glyphs are white on transparent: keep only the coverage (alpha) as a single channel
        std::vector<uint8_t> coverage(static_cast<std::size_t>(rgba->w) * static_cast<std::size_t>(rgba->h));
        const auto* pixels = static_cast<const uint8_t*>(rgba->pixels);
        for (int y = 0; y < rgba->h; ++y) {
            const uint8_t* row = pixels + static_cast<std::size_t>(y) * static_cast<std::size_t>(rgba->pitch);
            for (int x = 0; x < rgba->w; ++x) {
                coverage[static_cast<std::size_t>(y * rgba->w + x)] = row[x * 4 + 3];
            }
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, rgba->w, rgba->h, 0, GL_RED, GL_UNSIGNED_BYTE, coverage.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        SDL_DestroySurface(rgba);

        font_textures_[i] = texture;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::ensure_index_capacity(std::size_t quads)
{
    if (quads <= index_capacity_quads_) return;

    std::size_t capacity = index_capacity_quads_ ? index_capacity_quads_ : 4096;
    while (capacity < quads) capacity *= 2;

    std::vector<uint32_t> indices(capacity * 6);
    for (std::size_t q = 0; q < capacity; ++q) {
        const auto base = static_cast<uint32_t>(q * 4);
        uint32_t* idx = &indices[q * 6];
        idx[0] = base + 0; idx[1] = base + 1; idx[2] = base + 2;
        idx[3] = base + 0; idx[4] = base + 2; idx[5] = base + 3;
    }

    glBindVertexArray(vao_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    index_capacity_quads_ = capacity;
}

void Renderer::begin_frame(int32_t viewport_w, int32_t viewport_h, SDL_Color clear_color) noexcept
{
    glViewport(0, 0, viewport_w, viewport_h);
    glClearColor(clear_color.r / 255.0f, clear_color.g / 255.0f, clear_color.b / 255.0f, clear_color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glUniform2f(u_viewport_, static_cast<float>(viewport_w), static_cast<float>(viewport_h));
}

void Renderer::draw_grid(const DisplayGrid& grid, std::size_t font_index, int32_t offset_x, int32_t offset_y)
{
    last_vertex_count_ = 0;

    const int32_t width = grid.width();
    const int32_t height = grid.height();
    if (width <= 0 || height <= 0 || font_index >= font_sizes_.size()) return;

    const GLuint atlas = font_textures_[font_index];
    const std::size_t total_cells = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (bg_vertices_.size() < total_cells * 4) {
        bg_vertices_.resize(total_cells * 4);
        fg_vertices_.resize(total_cells * 4);
    }

    // Write quads straight into preallocated storage: no per-vertex push_back
    Vertex* bg = bg_vertices_.data();
    Vertex* fg = fg_vertices_.data();
    const Cell* cells = grid.data();
    const float f_font = static_cast<float>(font_sizes_[font_index]);
    constexpr float INV16 = 1.0f / 16.0f;

    for (int32_t cy = 0; cy < height; ++cy) {
        const float py0 = static_cast<float>(offset_y) + static_cast<float>(cy) * f_font;
        const float py1 = py0 + f_font;

        for (int32_t cx = 0; cx < width; ++cx) {
            const Cell& cell = *cells++;
            const float px0 = static_cast<float>(offset_x) + static_cast<float>(cx) * f_font;
            const float px1 = px0 + f_font;

            if (cell.bg.a > 0) {
                bg[0] = {px0, py0, -1.0f, -1.0f, cell.bg};
                bg[1] = {px1, py0, -1.0f, -1.0f, cell.bg};
                bg[2] = {px1, py1, -1.0f, -1.0f, cell.bg};
                bg[3] = {px0, py1, -1.0f, -1.0f, cell.bg};
                bg += 4;
            }

            if (cell.glyph != ' ' && cell.glyph != 0 && atlas != 0) {
                const float u0 = static_cast<float>(cell.glyph % 16) * INV16;
                const float v0 = static_cast<float>(cell.glyph / 16) * INV16;
                const float u1 = u0 + INV16;
                const float v1 = v0 + INV16;
                fg[0] = {px0, py0, u0, v0, cell.fg};
                fg[1] = {px1, py0, u1, v0, cell.fg};
                fg[2] = {px1, py1, u1, v1, cell.fg};
                fg[3] = {px0, py1, u0, v1, cell.fg};
                fg += 4;
            }
        }
    }

    const std::size_t bg_count = static_cast<std::size_t>(bg - bg_vertices_.data());
    const std::size_t fg_count = static_cast<std::size_t>(fg - fg_vertices_.data());
    const std::size_t total = bg_count + fg_count;
    if (total == 0) return;

    ensure_index_capacity(total / 4);

    // Backgrounds first, glyphs after, in one buffer -> one draw call preserves the layering
    const std::size_t bg_bytes = bg_count * sizeof(Vertex);
    const std::size_t fg_bytes = fg_count * sizeof(Vertex);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    if (bg_bytes + fg_bytes > vbo_capacity_bytes_) {
        vbo_capacity_bytes_ = total_cells * 8 * sizeof(Vertex);
    }
    // Re-specifying the storage orphans last frame's buffer, so the driver never stalls on in-flight data
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vbo_capacity_bytes_), nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bg_bytes), bg_vertices_.data());
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(bg_bytes), static_cast<GLsizeiptr>(fg_bytes), fg_vertices_.data());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glUniform1i(u_atlas_, 0);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>((total / 4) * 6), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    last_vertex_count_ = total;
}
