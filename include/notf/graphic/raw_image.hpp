#pragma once

#include <string>

#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// raw image ======================================================================================================== //

/// Helper structure around raw image data.
/// Raw images are usually loaded from disk, have their data copied into another object and are then destroyed again.
/// Use-cases include loading OpenGL textures and Window icons.
class RawImage {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value Constructor.
    /// @param image_path       Absolute path to the Image file.
    /// @param force_format     Number of bytes per pixel, default is 0 (=> use format defined in file).
    /// @throw ResourceError    If the Image fails to load.
    RawImage(std::string image_path, int force_format = 0);

    /// Destructor
    ~RawImage();

    /// Width of the image in pixels.
    int get_width() const { return m_width; }

    /// Height of the image in pixels.
    int get_height() const { return m_height; }

    /// Number of channels per pixel.
    int get_channels() const { return m_channels; }

    /// Absolute path to the file from which the image was loaded.
    const std::string& get_filepath() const { return m_filepath; }

    /// Raw image data.
    const uchar* get_data() const { return m_data; }

    /// Tests if the image is valid or not.
    explicit operator bool() const { return static_cast<bool>(m_data); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Absolute path to the file from which the image was loaded.
    const std::string m_filepath;

    /// Width of the image in pixels.
    int m_width = 0;

    /// Height of the image in pixels.
    int m_height = 0;

    /// Number of channels per pixel.
    int m_channels = 0;

    /// Raw image data.
    uchar* m_data = nullptr;
};

NOTF_CLOSE_NAMESPACE
