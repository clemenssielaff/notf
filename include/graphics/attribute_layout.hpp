#pragma once

#include <string>
#include <vector>

#include "./gl_forwards.hpp"

namespace notf {

class AttributeLayout {

    // types ----------------------------------------------------------------------------------------------------------/
public:
    /// @brief Attribute definition.
    struct Attribute {

        // @brief Index of the variable - is not the same as its location.
        GLuint index;

        /// @brief Location of the variable, used to address the variable in the OpenGL shader program.
        GLint location;

        /// @brief Type of the variable.
        /// @note See https://www.khronos.org/opengl/wiki/GLAPI/glGetActiveUniform#Description for details.
        GLenum type;

        /// @brief Number of elements in the variable in units of type.
        /// @note Is always >=1 and only >1 if the variable is an array.
        GLint size;

        /// @brief The name of the variable.
        std::string name;
    };

    // methods --------------------------------------------------------------------------------------------------------/
public:
    /// @param attributes   Attributes
    AttributeLayout(std::vector<Attribute>&& attributes) : m_attributes(std::move(attributes)) {}

    /// @brief All attributes.
    const std::vector<Attribute>& attributes() const { return m_attributes; }

    /// @brief Equality operator.
    bool operator==(const AttributeLayout& other) const { return m_attributes == other.m_attributes; }

    /// @brief Inequality operator.
    bool operator!=(const AttributeLayout& other) const { return m_attributes != other.m_attributes; }

    // fields ---------------------------------------------------------------------------------------------------------/
private:
    /// @brief Attributes in order.
    std::vector<Attribute> m_attributes;
};

} // namespace notf
