#pragma once

#include <string>
#include <vector>

#include "./gl_forwards.hpp"

namespace notf {

class AttributeLayout {

    // types ----------------------------------------------------------------------------------------------------------/
public:
    /// Attribute definition.
    struct Attribute {

        // Index of the variable - is not the same as its location.
        GLuint index;

        /// Location of the variable, used to address the variable in the OpenGL shader program.
        GLint location;

        /// Type of the variable.
        /// @note See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
        GLenum type;

        /// Number of elements in the variable in units of type.
        /// @note Is always >=1 and only >1 if the variable is an array.
        GLint size;

        /// The name of the variable.
        std::string name;
    };

    // methods --------------------------------------------------------------------------------------------------------/
public:
    /// @param attributes   Attributes
    AttributeLayout(std::vector<Attribute>&& attributes) : m_attributes(std::move(attributes)) {}

    /// All attributes.
    const std::vector<Attribute>& attributes() const { return m_attributes; }

    /// Equality operator.
    bool operator==(const AttributeLayout& other) const { return m_attributes == other.m_attributes; }

    /// Inequality operator.
    bool operator!=(const AttributeLayout& other) const { return m_attributes != other.m_attributes; }

    // fields ---------------------------------------------------------------------------------------------------------/
private:
    /// Attributes in order.
    std::vector<Attribute> m_attributes;
};

} // namespace notf
