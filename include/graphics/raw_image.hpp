#pragma once

#include <string>

namespace notf {

using uchar = unsigned char;

/**
 * Helper structure around raw image data.
 *
 * Image%s are not managed by the ResourceManager.
 * Instead, they are usually loaded from disk, have their data copied into another object and are then destroyed again.
 * Use-cases include loading OpenGL textures and Window icons.
 */
class RawImage final {

public: // methods
    /**
     * @param image_path            Absolute path to the Image file.
     * @param force_format          Number of bytes per pixel, default is 0 (use defined in file).
     * @throw std::runtime_error    If the Image fails to load.
     */
    explicit RawImage(const std::string& image_path, int force_format = 0);

    ~RawImage();

    /** Width of the Image in pixels. */
    int get_width() const { return m_width; }

    /** Height of the Image in pixels. */
    int get_height() const { return m_height; }

    /** Bytes per pixel. */
    int get_bytes_per_pixel() const { return m_bytes; }

    /** Absolute path to the file from which the Image was loaded. */
    const std::string& get_filepath() const { return m_filepath; }

    /** Raw image data. */
    const uchar* get_data() const { return m_data; }

    /** Tests if the Image is valid or not. */
    explicit operator bool() const { return static_cast<bool>(m_data); }

private: // fields
    /** Width of the Image in pixels. */
    int m_width;

    /** Height of the Image in pixels. */
    int m_height;

    /** Bytes per pixel. */
    int m_bytes;

    /** Absolute path to the file from which the Image was loaded. */
    std::string m_filepath;

    /** Raw image data. */
    uchar* m_data;
};

} // namespace notf
