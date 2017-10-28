#include "graphics/graphics_context.hpp"

#include <assert.h>
#include <cstring>
#include <set>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/glfw.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"

namespace { // anonymous),

const uint ALL_ZEROS = 0x00000000;
const uint ALL_ONES  = 0xffffffff;

} // namespace

//====================================================================================================================//

#if NOTF_LOG_LEVEL >= NOTF_LOG_LEVEL_INFO
#define CHECK_EXTENSION(member, name)                                                                              \
	member = extensions.count(name);                                                                               \
	if (member) {                                                                                                  \
		notf::LogMessageFactory(notf::LogMessage::LEVEL::INFO, __LINE__, notf::basename(__FILE__), "GLExtensions") \
		        .input                                                                                             \
		    << "Found OpenGL extension:\"" << name << "\"";                                                        \
	}
#else
#define CHECK_EXTENSION(member, name) member = extensions.count(name);
#endif

namespace notf {

GraphicsContext::Extensions::Extensions() {
	std::set<std::string> extensions;
	{ // create a set of all available extensions
		GLint extension_count;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
		for (GLint i = 0; i < extension_count; ++i) {
			extensions.emplace(
			    std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)))));
		}
	}

	// initialize the members
	CHECK_EXTENSION(anisotropic_filter, "GL_EXT_texture_filter_anisotropic");
}

#undef CHECK_EXTENSION

//====================================================================================================================//

GraphicsContext::State::State() {
	{ // texture slots
		GLint texture_slot_count = 0;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_slot_count);
		assert(texture_slot_count > 0);
		texture_slots = std::vector<Texture2Ptr>(static_cast<size_t>(texture_slot_count));
	}
}

//====================================================================================================================//

GraphicsContext::GraphicsContext(GLFWwindow* window)
    : m_window(window)
    , m_current_thread(0)
    , m_has_vsync(true)
    , m_stencil_func(StencilFunc::DEFAULT)
    , m_stencil_mask(0)
    , m_blend_mode(BlendMode::DEFAULT)
    , m_textures()
    , m_shader_stack()
    , m_shaders() {
	if (!window) {
		throw_runtime_error("Failed to create a new GraphicsContext without a window (given pointer is null).");
	}

	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(m_has_vsync ? 1 : 0);
}

GraphicsContext::~GraphicsContext() {
	// deallocate and invalidate all remaining Textures
	for (std::weak_ptr<Texture2> texture_weakptr : m_textures) {
		std::shared_ptr<Texture2> texture = texture_weakptr.lock();
		if (texture) {
			log_warning << "Deallocating live Texture: \"" << texture->name() << "\"";
			texture->_deallocate();
		}
	}
	m_textures.clear();

	// deallocate and invalidate all remaining Shaders
	for (std::weak_ptr<Shader> shader_weakptr : m_shaders) {
		std::shared_ptr<Shader> shader = shader_weakptr.lock();
		if (shader) {
			log_warning << "Deallocating live Shader: \"" << shader->name() << "\"";
			shader->_deallocate();
		}
	}
	m_shaders.clear();
}

const GraphicsContext::Extensions& GraphicsContext::extensions() const {
	static const Extensions singleton;
	return singleton;
}

void GraphicsContext::set_vsync(const bool enabled) {
	if (enabled != m_has_vsync) {
		m_has_vsync = enabled;
		glfwSwapInterval(m_has_vsync ? 1 : 0);
	}
}

void GraphicsContext::set_stencil_func(const StencilFunc func) {
	if (func == m_stencil_func) {
		return;
	}
	m_stencil_func = func;

	switch (func) {
		case StencilFunc::ALWAYS:
			glStencilFunc(GL_ALWAYS, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::NEVER:
			glStencilFunc(GL_NEVER, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::LESS:
			glStencilFunc(GL_LESS, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::LEQUAL:
			glStencilFunc(GL_LEQUAL, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::GREATER:
			glStencilFunc(GL_GREATER, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::GEQUAL:
			glStencilFunc(GL_GEQUAL, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::EQUAL:
			glStencilFunc(GL_EQUAL, ALL_ZEROS, ALL_ONES);
			break;
		case StencilFunc::NOTEQUAL:
			glStencilFunc(GL_NOTEQUAL, ALL_ZEROS, ALL_ONES);
			break;
	}
}

void GraphicsContext::set_stencil_mask(const GLuint mask) {
	if (mask != m_stencil_mask) {
		m_stencil_mask = mask;
		glStencilMask(mask);
	}
}

void GraphicsContext::set_blend_mode(const BlendMode mode) {
	if (mode == m_blend_mode) {
		return;
	}
	m_blend_mode = mode;

	// color
	GLenum rgb_sfactor;
	GLenum rgb_dfactor;
	switch (mode.rgb) {
		case (BlendMode::SOURCE_OVER):
			rgb_sfactor = GL_ONE;
			rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::SOURCE_IN):
			rgb_sfactor = GL_DST_ALPHA;
			rgb_dfactor = GL_ZERO;
			break;
		case (BlendMode::SOURCE_OUT):
			rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
			rgb_dfactor = GL_ZERO;
			break;
		case (BlendMode::SOURCE_ATOP):
			rgb_sfactor = GL_DST_ALPHA;
			rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_OVER):
			rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
			rgb_dfactor = GL_ONE;
			break;
		case (BlendMode::DESTINATION_IN):
			rgb_sfactor = GL_ZERO;
			rgb_dfactor = GL_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_OUT):
			rgb_sfactor = GL_ZERO;
			rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_ATOP):
			rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
			rgb_dfactor = GL_SRC_ALPHA;
			break;
		case (BlendMode::LIGHTER):
			rgb_sfactor = GL_ONE;
			rgb_dfactor = GL_ONE;
			break;
		case (BlendMode::COPY):
			rgb_sfactor = GL_ONE;
			rgb_dfactor = GL_ZERO;
			break;
		case (BlendMode::XOR):
			rgb_sfactor = GL_ONE_MINUS_DST_ALPHA;
			rgb_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		default:
			rgb_sfactor = 0;
			rgb_dfactor = 0;
			assert(false);
			break;
	}

	// alpha
	GLenum alpha_sfactor;
	GLenum alpha_dfactor;
	switch (mode.alpha) {
		case (BlendMode::SOURCE_OVER):
			alpha_sfactor = GL_ONE;
			alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::SOURCE_IN):
			alpha_sfactor = GL_DST_ALPHA;
			alpha_dfactor = GL_ZERO;
			break;
		case (BlendMode::SOURCE_OUT):
			alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
			alpha_dfactor = GL_ZERO;
			break;
		case (BlendMode::SOURCE_ATOP):
			alpha_sfactor = GL_DST_ALPHA;
			alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_OVER):
			alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
			alpha_dfactor = GL_ONE;
			break;
		case (BlendMode::DESTINATION_IN):
			alpha_sfactor = GL_ZERO;
			alpha_dfactor = GL_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_OUT):
			alpha_sfactor = GL_ZERO;
			alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case (BlendMode::DESTINATION_ATOP):
			alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
			alpha_dfactor = GL_SRC_ALPHA;
			break;
		case (BlendMode::LIGHTER):
			alpha_sfactor = GL_ONE;
			alpha_dfactor = GL_ONE;
			break;
		case (BlendMode::COPY):
			alpha_sfactor = GL_ONE;
			alpha_dfactor = GL_ZERO;
			break;
		case (BlendMode::XOR):
			alpha_sfactor = GL_ONE_MINUS_DST_ALPHA;
			alpha_dfactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		default:
			alpha_sfactor = 0;
			alpha_dfactor = 0;
			assert(false);
			break;
	}
	glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor);
}

void GraphicsContext::bind_texture(Texture2Ptr texture, uint slot) {
	if (!texture) {
		return clear_texture(slot);
	}
	if (!texture->is_valid()) {
		throw_runtime_error(string_format("Cannot bind invalid texture \"%s\"", texture->name().c_str()));
	}

	if (m_texture_stack.empty() || texture != m_texture_stack.back()) {
		//        glActiveTexture(GL_TEXTURE0); // TODO: allow several active textures
		glBindTexture(GL_TEXTURE_2D, texture->id());
	}
	m_texture_stack.emplace_back(std::move(texture));
}

void GraphicsContext::clear_texture(uint slot) {
	if (m_texture_stack.empty()) {
		return; // ignore calls on an empty stack
	}

	m_texture_stack.clear();
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GraphicsContext::bind_shader(ShaderPtr shader) {
	if (!shader->is_valid()) {
		throw_runtime_error(string_format("Cannot bind invalid shader \"%s\"", shader->name().c_str()));
	}

	if (m_shader_stack.empty() || shader != m_shader_stack.back()) {
		glUseProgram(shader->id());
	}
	m_shader_stack.emplace_back(std::move(shader));
}

void GraphicsContext::clear_shader() {
	if (m_shader_stack.empty()) {
		return; // ignore calls on an empty stack
	}

	m_shader_stack.clear();
	glUseProgram(0);
}

void GraphicsContext::release_shader_compiler() {
	glReleaseShaderCompiler();
	check_gl_error();
}

} // namespace notf
