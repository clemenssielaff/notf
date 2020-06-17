#include "glad.h"

#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "fmt/format.h"

#include "utf8.h"

#include "notf/meta/bits.hpp"
#include "notf/meta/enum.hpp"
#include "notf/meta/numeric.hpp"

#include "notf/common/color.hpp"
#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/common/geo/size2.hpp"

#include "docstr.hpp"

#include <optional>
#include <stdexcept>

// UTILS ============================================================================================================ //

const char* substring_end(const std::string& string, const int max_codepoints) {
    const char* itr = string.c_str();
    const char* const end = itr + string.size();
    if (max_codepoints < 0) { return end; }
    int count = 0;
    try {
        for (; count < max_codepoints && itr < end; ++count) {
            utf8::next(itr, end);
        }
    }
    catch (const utf8::invalid_code_point& exception) {
        throw std::runtime_error(fmt::format("Encountered invalid UTF8 codepoint in string \"{}\"", string));
    }
    return itr;
}

NVGcolor notf_to_nvg_color(const notf::Color& color) { return notf::bit_cast<NVGcolor>(color); }

// NANOVG =========================================================================================================== //

struct NanoVG;

enum class Winding : int {
    CCW = 1,
    CW = 2,
    COUNTERCLOCKWISE = CCW,
    CLOCKWISE = CW,
    SOLID = CCW,
    HOLE = CW,
};

enum class LineCap : int {
    BUTT = 0,
    ROUND = 1,
    SQUARE = 2,
    BEVEL = 2,
    MITER = 0,
};

enum Align : int {
    // Horizontal align
    LEFT = 1 << 0,   // Default, align text horizontally to left.
    CENTER = 1 << 1, // Align text horizontally to center.
    RIGHT = 1 << 2,  // Align text horizontally to right.

    // Vertical align
    TOP = 1 << 3,      // Align text vertically to top.
    MIDDLE = 1 << 4,   // Align text vertically to middle.
    BOTTOM = 1 << 5,   // Align text vertically to bottom.
    BASELINE = 1 << 6, // Default, align text vertically to baseline.
};

enum BlendFactor : int {
    ZERO = 1 << 0,
    ONE = 1 << 1,
    SRC_COLOR = 1 << 2,
    ONE_MINUS_SRC_COLOR = 1 << 3,
    DST_COLOR = 1 << 4,
    ONE_MINUS_DST_COLOR = 1 << 5,
    SRC_ALPHA = 1 << 6,
    ONE_MINUS_SRC_ALPHA = 1 << 7,
    DST_ALPHA = 1 << 8,
    ONE_MINUS_DST_ALPHA = 1 << 9,
    SRC_ALPHA_SATURATE = 1 << 10,
};

enum class CompositeOperation : int {
    SOURCE_OVER,
    SOURCE_IN,
    SOURCE_OUT,
    ATOP,
    DESTINATION_OVER,
    DESTINATION_IN,
    DESTINATION_OUT,
    DESTINATION_ATOP,
    LIGHTER,
    COPY,
    XOR,
};

enum ImageFlags : int {
    GENERATE_MIPMAPS = 1 << 0, // Generate mipmaps during creation of the image.
    REPEAT_X = 1 << 1,         // Repeat image in X direction.
    REPEAT_Y = 1 << 2,         // Repeat image in Y direction.
    FLIP_Y = 1 << 3,           // Flips (inverses) image in Y direction when rendered.
    PREMULTIPLIED = 1 << 4,    // Image data has premultiplied alpha.
    NEAREST = 1 << 5,          // Image interpolation is Nearest instead Linear
};

struct Image {
    Image(NanoVG& context, const std::string& file_path, ImageFlags flags);
    ~Image();
    notf::Size2i get_size() const;

    std::weak_ptr<NanoVG> nanovg;
    int id = 0;
};
using ImagePtr = std::shared_ptr<Image>;

struct Paint {
    notf::M3f xform;
    notf::Size2f extent;
    float radius;
    float feather;
    notf::Color inner_color;
    notf::Color outer_color;
    ImagePtr image;

    static Paint from_nvg_paint(NVGpaint paint) {
        Paint result;
        std::copy(result.xform.as_ptr(), result.xform.as_ptr() + sizeof(float) * 6, paint.xform);
        std::copy(result.extent.as_ptr(), result.extent.as_ptr() + sizeof(float) * 2, paint.extent);
        result.radius = paint.radius;
        result.feather = paint.feather;
        result.inner_color = notf::bit_cast<notf::Color>(paint.innerColor);
        result.outer_color = notf::bit_cast<notf::Color>(paint.outerColor);
        return result;
    }

    NVGpaint to_nvg_paint() const {
        NVGpaint result;
        std::copy(&result.xform[0], &result.xform[0] + sizeof(float) * 6, const_cast<float*>(xform.as_ptr()));
        std::copy(&result.extent[0], &result.extent[0] + sizeof(float) * 2, const_cast<float*>(extent.as_ptr()));
        result.radius = radius;
        result.feather = feather;
        result.innerColor = notf_to_nvg_color(inner_color);
        result.outerColor = notf_to_nvg_color(outer_color);
        result.image = image ? image->id : 0;
        return result;
    }
};

struct Font {
    int id = 0;
};

struct GlyphPosition {
    uint index = 0;
    float x_advance = 0;
    float x_min = 0;
    float x_max = 0;
};

struct TextRow {
    std::string text;
    uint first_char_index = 0;
    float width = 0;
    float x_min = 0;
    float x_max = 0;
};

struct FontMetrics {
    float ascender = 0;
    float descender = 0;
    float line_height = 0;
};

/// nanovg pseudo object to be used in the Python bindings.
struct NanoVG : public std::enable_shared_from_this<NanoVG> {
    NanoVG() : ctx(nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES)) {
        if (!ctx) { throw std::runtime_error("Failed to create OpenGLES 3 context"); }
    }

    ~NanoVG() {
        if (ctx) {
            nvgDeleteGLES3(ctx);
            ctx = nullptr;
        }
    }

    void begin_frame(const float window_width, const float window_height, const float device_pixel_ratio = 1.f) {
        nvgBeginFrame(ctx, notf::max(0.f, window_width), notf::max(0.f, window_height), device_pixel_ratio);
    }

    void cancel_frame() { nvgCancelFrame(ctx); }

    void end_frame() { nvgEndFrame(ctx); }

    void save() { nvgSave(ctx); }

    void restore() { nvgRestore(ctx); }

    void reset() { nvgReset(ctx); }

    void shape_anti_alias(const bool enabled) { nvgShapeAntiAlias(ctx, enabled ? 1 : 0); }

    void stroke_color(const notf::Color color) { nvgStrokeColor(ctx, notf_to_nvg_color(color)); }

    void stroke_paint(const Paint paint) { nvgStrokePaint(ctx, paint.to_nvg_paint()); }

    void fill_color(const notf::Color color) { nvgFillColor(ctx, notf_to_nvg_color(color)); }

    void fill_paint(const Paint paint) { nvgFillPaint(ctx, paint.to_nvg_paint()); }

    void miter_limit(const float limit) { nvgMiterLimit(ctx, notf::max(0.f, limit)); }

    void stroke_width(const float width) { nvgStrokeWidth(ctx, notf::max(0.f, width)); }

    void line_cap(const LineCap cap) { nvgLineCap(ctx, notf::to_number(cap)); }

    void line_join(const LineCap join) { nvgLineJoin(ctx, notf::to_number(join)); }

    void global_alpha(const float alpha) { nvgGlobalAlpha(ctx, notf::clamp(alpha)); }

    void global_composite_operation(const CompositeOperation operation) {
        nvgGlobalCompositeOperation(ctx, notf::to_number(operation));
    }

    void global_composite_blend_func(const BlendFactor src_factor, const BlendFactor dst_factor) {
        nvgGlobalCompositeBlendFunc(ctx, notf::to_number(src_factor), notf::to_number(dst_factor));
    }

    void global_composite_blend_func_separate(const BlendFactor src_rgb, const BlendFactor dst_rgb,
                                              const BlendFactor src_alpha, const BlendFactor dst_alpha) {
        nvgGlobalCompositeBlendFuncSeparate(ctx, notf::to_number(src_rgb), notf::to_number(dst_rgb),
                                            notf::to_number(src_alpha), notf::to_number(dst_alpha));
    }

    void reset_transform() { nvgResetTransform(ctx); }

    void transform(const float a, const float b, const float c, const float d, const float e, const float f) {
        nvgTransform(ctx, a, b, c, d, e, f);
    }

    void translate(const float x, const float y) { nvgTranslate(ctx, x, y); }

    void rotate(const float angle) { nvgRotate(ctx, angle); }

    void skew_x(const float angle) { nvgSkewX(ctx, angle); }

    void skew_y(const float angle) { nvgSkewY(ctx, angle); }

    void scale(const float x, const float y) { nvgScale(ctx, x, y); }

    notf::M3f current_transform() {
        notf::M3f result;
        nvgCurrentTransform(ctx, result.as_ptr());
        return result;
    }

    //    void transform_identity(float* dst) {
    //        return nvgTransformIdentity(dst);
    //    }

    //    void transform_translate(float* dst, float tx, float ty) {
    //        return nvgTransformTranslate(dst, tx, ty);
    //    }

    //    void transform_scale(float* dst, float sx, float sy) {
    //        return nvgTransformScale(dst, sx, sy);
    //    }

    //    void transform_rotate(float* dst, float a) {
    //        return nvgTransformRotate(dst, a);
    //    }

    //    void transform_skew_x(float* dst, float a) {
    //        return nvgTransformSkewX(dst, a);
    //    }

    //    void transform_skew_y(float* dst, float a) {
    //        return nvgTransformSkewY(dst, a);
    //    }

    //    void transform_multiply(float* dst, const float* src) {
    //        return nvgTransformMultiply(dst, src);
    //    }

    //    void transform_premultiply(float* dst, const float* src) {
    //        return nvgTransformPremultiply(dst, src);
    //    }

    //    int transform_inverse(float* dst, const float* src) {
    //        return nvgTransformInverse(dst, src);
    //    }

    //    void transform_point(float* dstx, float* dsty, const float* xform, float srcx, float srcy) {
    //        return nvgTransformPoint(dstx, dsty, xform, srcx, srcy);
    //    }

    //    float deg_to_rad(float deg) {
    //        return nvgDegToRad(deg);
    //    }

    //    float rad_to_deg(float rad) {
    //        return nvgRadToDeg(rad);
    //    }

    ImagePtr create_image(const std::string& file_path, const ImageFlags flags) {
        return std::make_shared<Image>(*this, file_path, flags);
    }

    //    int create_image_mem(int image_flags, unsigned char* data, int ndata) {
    //        return nvgCreateImageMem(ctx, image_flags, data, ndata);
    //    }

    //    int create_image_rgba(int w, int h, int image_flags, const unsigned char* data) {
    //        return nvgCreateImageRGBA(ctx, w, h, image_flags, data);
    //    }

    //    void update_image(int image, const unsigned char* data) {
    //        return nvgUpdateImage(ctx, image, data);
    //    }

    Paint linear_gradient(const float start_x, const float start_y, const float end_x, const float end_y,
                          const notf::Color start_color, const notf::Color end_color) {
        return Paint::from_nvg_paint(nvgLinearGradient(ctx, start_x, start_y, end_x, end_y,
                                                       notf_to_nvg_color(start_color), notf_to_nvg_color(end_color)));
    }

    Paint box_gradient(const float left, const float top, const float width, const float height, const float radius,
                       const float feather, const notf::Color start_color, const notf::Color end_color) {
        return Paint::from_nvg_paint(nvgBoxGradient(ctx, left, top, width, height, radius, feather,
                                                    notf_to_nvg_color(start_color), notf_to_nvg_color(end_color)));
    }

    Paint radial_gradient(const float center_x, const float center_y, const float inner_radius,
                          const float outer_radius, const notf::Color start_color, const notf::Color end_color) {
        return Paint::from_nvg_paint(nvgRadialGradient(ctx, center_x, center_y, inner_radius, outer_radius,
                                                       notf_to_nvg_color(start_color), notf_to_nvg_color(end_color)));
    }

    Paint image_pattern(const float left, const float top, const float width, const float height, const float angle,
                        const ImagePtr& image, const float alpha) {
        Paint result = Paint::from_nvg_paint(
            nvgImagePattern(ctx, left, top, width, height, angle, image->id, notf::clamp(alpha)));
        result.image = image;
        return result;
    }

    void scissor(const float left, const float top, const float width, const float height) {
        nvgScissor(ctx, left, top, width, height);
    }

    void intersect_scissor(const float left, const float top, const float width, const float height) {
        nvgIntersectScissor(ctx, left, top, width, height);
    }

    void reset_scissor() { nvgResetScissor(ctx); }

    void begin_path() { nvgBeginPath(ctx); }

    void move_to(const float x, const float y) { nvgMoveTo(ctx, x, y); }

    void line_to(const float x, const float y) { nvgLineTo(ctx, x, y); }

    void bezier_to(const float ctrl1_x, const float ctrl1_y, const float ctrl2_x, const float ctrl2_y,
                   const float end_x, const float end_y) {
        nvgBezierTo(ctx, ctrl1_x, ctrl1_y, ctrl2_x, ctrl2_y, end_x, end_y);
    }

    void quad_to(const float ctrl_x, const float ctrl_y, const float end_x, const float end_y) {
        nvgQuadTo(ctx, ctrl_x, ctrl_y, end_x, end_y);
    }

    void arc_to(const float x1, const float y1, const float x2, const float y2, const float radius) {
        nvgArcTo(ctx, x1, y1, x2, y2, radius);
    }

    void close_path() { nvgClosePath(ctx); }

    void path_winding(const Winding winding) { nvgPathWinding(ctx, notf::to_number(winding)); }

    void arc(const float center_x, const float center_y, const float arc_radius, const float angle_start,
             const float angle_end, const Winding direction) {
        nvgArc(ctx, center_x, center_y, arc_radius, angle_start, angle_end, notf::to_number(direction));
    }

    void rect(const float left, const float top, const float width, const float height) {
        nvgRect(ctx, left, top, width, height);
    }

    void rounded_rect(const float left, const float top, const float width, const float height, const float radius) {
        nvgRoundedRect(ctx, left, top, width, height, radius);
    }

    void rounded_rect_varying(const float left, const float top, const float width, const float height,
                              const float radius_top_left, const float radius_top_right,
                              const float radius_bottom_right, const float radius_bottom_left) {
        nvgRoundedRectVarying(ctx, left, top, width, height, radius_top_left, radius_top_right, radius_bottom_right,
                              radius_bottom_left);
    }

    void ellipse(const float center_x, const float center_y, const float radius_x, const float radius_y) {
        nvgEllipse(ctx, center_x, center_y, radius_x, radius_y);
    }

    void circle(const float center_x, const float center_y, const float radius) {
        nvgCircle(ctx, center_x, center_y, radius);
    }

    void fill() { nvgFill(ctx); }

    void stroke() { nvgStroke(ctx); }

    Font create_font(const std::string& name, const std::string& file_path, const int font_index = -1) {
        int result;
        if (font_index == -1) {
            result = nvgCreateFont(ctx, name.c_str(), file_path.c_str());
            if (result == -1) { throw std::runtime_error(fmt::format("Failed to load font \"{}\"", file_path)); }
        } else {
            result = nvgCreateFontAtIndex(ctx, name.c_str(), file_path.c_str(), font_index);
            if (result == -1) {
                throw std::runtime_error(
                    fmt::format("Failed to load font at index {} from file \"{}\"", font_index, file_path));
            }
        }
        return Font{result};
    }
    //    int create_font_mem(const char* name, unsigned char* data, int ndata, int free_data) {
    //        return nvgCreateFontMem(ctx, name, data, ndata, free_data);
    //    }

    //    int create_font_mem_at_index(const char* name, unsigned char* data, int ndata, int free_data, const int
    //    font_index) {
    //        return nvgCreateFontMemAtIndex(ctx, name, data, ndata, free_data, font_index);
    //    }

    std::optional<Font> find_font(const std::string& name) {
        const int result = nvgFindFont(ctx, name.c_str());
        if (result == -1) {
            return {};
        } else {
            return Font{result};
        }
    }

    bool add_fallback_font(const Font base, const Font fallback) {
        return nvgAddFallbackFontId(ctx, base.id, fallback.id) == 1;
    }

    void reset_fallback_fonts(const Font base) { nvgResetFallbackFontsId(ctx, base.id); }

    void font_size(const float size = 16.f) { nvgFontSize(ctx, notf::max(0.f, size)); }

    void font_blur(const float blur = 0.f) { nvgFontBlur(ctx, notf::max(0.f, blur)); }

    void text_letter_spacing(const float spacing = 0.f) { nvgTextLetterSpacing(ctx, notf::max(spacing)); }

    void text_line_height(const float line_height = 1.f) { nvgTextLineHeight(ctx, line_height); }

    void text_align(const Align align = Align(Align::LEFT | Align::BASELINE)) {
        nvgTextAlign(ctx, notf::to_number(align));
    }

    void font_face(const Font font) { nvgFontFaceId(ctx, font.id); }

    float text(const float x, const float y, const std::string& string, const int char_count = -1) {
        if (char_count == 0) { return 0; }
        return nvgText(ctx, x, y, string.c_str(), substring_end(string, char_count));
    }

    void text_box(const float x, const float y, const std::string& string, const float width,
                  const int char_count = -1) {
        if (char_count == 0) { return; }
        nvgTextBox(ctx, x, y, width, string.c_str(), substring_end(string, char_count));
    }

    std::tuple<notf::Aabrf, float> text_bounds(const float x, const float y, const std::string& string,
                                               const int char_count = -1) {
        if (char_count == 0) { return std::make_tuple(notf::Aabrf::zero(), 0.f); }
        notf::Aabrf aabr;
        const float horizontal_advance
            = nvgTextBounds(ctx, x, y, string.c_str(), substring_end(string, char_count), aabr.as_ptr());
        return std::make_tuple(aabr, horizontal_advance);
    }

    notf::Aabrf text_box_bounds(const float x, const float y, const std::string& string, const float width,
                                const int char_count = -1) {
        if (char_count == 0) { return notf::Aabrf::zero(); }
        notf::Aabrf aabr;
        nvgTextBoxBounds(ctx, x, y, width, string.c_str(), substring_end(string, char_count), aabr.as_ptr());
        return aabr;
    }

    std::vector<GlyphPosition> text_glyph_positions(const float x, const float y, const std::string& string,
                                                    int char_count = -1) {
        if (char_count == 0) { return {}; }

        char_count = notf::min(char_count, notf::narrow_cast<int>(string.size()));
        std::vector<NVGglyphPosition> nvg_glyphs(static_cast<uint>(char_count));
        const uint actual_glyph_count = static_cast<uint>(nvgTextGlyphPositions(
            ctx, x, y, string.c_str(), string.c_str() + string.size(), nvg_glyphs.data(), char_count));

        std::vector<GlyphPosition> result;
        result.reserve(actual_glyph_count);
        for (uint index = 0; index < actual_glyph_count; ++index) {
            const NVGglyphPosition& nvg_glyph = nvg_glyphs[index];
            result.emplace_back(GlyphPosition{index++, nvg_glyph.x, nvg_glyph.minx, nvg_glyph.maxx});
        }
        return result;
    }

    FontMetrics text_metrics() {
        FontMetrics result;
        nvgTextMetrics(ctx, &result.ascender, &result.descender, &result.line_height);
        return result;
    }

    std::vector<TextRow> text_break_lines(const std::string& string, const float width, const int char_count = -1,
                                          const uint max_rows = 100) {
        std::vector<NVGtextRow> nvg_rows(max_rows);
        const uint actual_row_count = static_cast<uint>(nvgTextBreakLines(
            ctx, string.c_str(), substring_end(string, char_count), width, nvg_rows.data(), max_rows));

        std::vector<TextRow> result;
        result.reserve(actual_row_count);
        for (uint index = 0; index < actual_row_count; ++index) {
            const NVGtextRow& nvg_row = nvg_rows[index];
            const uint first_char_index = notf::narrow_cast<uint>(std::distance(string.c_str(), nvg_row.start));
            result.emplace_back(TextRow{string.substr(first_char_index, std::distance(nvg_row.start, nvg_row.end)),
                                        first_char_index, nvg_row.width, nvg_row.minx, nvg_row.maxx});
        }
        return result;
    }

    //    static NVGcolor rgb(const unsigned char r, const unsigned char g, const unsigned char b) { return nvgRGB(r, g,
    //    b); }

    //    static NVGcolor rg_bf(const float r, const float g, const float b) { return nvgRGBf(r, g, b); }

    //    static NVGcolor rgba(const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char
    //    a) {
    //        return nvgRGBA(r, g, b, a);
    //    }

    //    static NVGcolor rgb_af(const float r, const float g, const float b, const float a) { return nvgRGBAf(r, g, b,
    //    a); }

    //    static NVGcolor lerp_rgba(const NVGcolor c0, const NVGcolor c1, const float u) { return nvgLerpRGBA(c0, c1,
    //    u); }

    //    static NVGcolor trans_rgba(const NVGcolor c0, const unsigned char a) { return nvgTransRGBA(c0, a); }

    //    static NVGcolor trans_rgb_af(const NVGcolor c0, const float a) { return nvgTransRGBAf(c0, a); }

    //    static NVGcolor hsl(const float h, const float s, const float l) { return nvgHSL(h, s, l); }

    //    static NVGcolor hsla(const float h, const float s, const float l, const unsigned char a) {
    //        return nvgHSLA(h, s, l, a);
    //    }

    NVGcontext* ctx = nullptr;
};

Image::Image(NanoVG& context, const std::string& file_path, ImageFlags flags)
    : nanovg(context.weak_from_this()), id(nvgCreateImage(context.ctx, file_path.c_str(), notf::to_number(flags))) {
    if (id == 0) { throw std::runtime_error(fmt::format("Failed to load image \"{}\"", file_path)); }
}

Image::~Image() {
    if (id) {
        if (std::shared_ptr<NanoVG> context = nanovg.lock()) { nvgDeleteImage(context->ctx, id); }
    }
}

notf::Size2i Image::get_size() const {
    assert(id);
    notf::Size2i result;
    if (std::shared_ptr<NanoVG> context = nanovg.lock()) {
        nvgImageSize(context->ctx, id, &result.data[0], &result.data[1]);
    }
    return result;
}

// clang-format off

// PYNANOVG ========================================================================================================= //

void produce_nanovg(pybind11::module& module) {

    py::enum_<Winding>(module, "Winding", py::arithmetic())
        .value("CCW", Winding::CCW)
        .value("CW", Winding::CW)
        .value("COUNTERCLOCKWISE", Winding::COUNTERCLOCKWISE)
        .value("CLOCKWISE", Winding::CLOCKWISE)
        .value("SOLID", Winding::SOLID)
        .value("HOLE", Winding::HOLE);

    py::enum_<LineCap>(module, "LineCap", py::arithmetic())
        .value("BUTT", LineCap::BUTT)
        .value("ROUND", LineCap::ROUND)
        .value("SQUARE", LineCap::SQUARE)
        .value("BEVEL", LineCap::BEVEL)
        .value("MITER", LineCap::MITER);

    py::enum_<Align>(module, "Align", py::arithmetic())
        .value("LEFT", Align::LEFT)
        .value("CENTER", Align::CENTER)
        .value("RIGHT", Align::RIGHT)
        .value("TOP", Align::TOP)
        .value("MIDDLE", Align::MIDDLE)
        .value("BOTTOM", Align::BOTTOM)
        .value("BASELINE", Align::BASELINE);

    py::enum_<BlendFactor>(module, "BlendFactor", py::arithmetic())
        .value("ZERO", BlendFactor::ZERO)
        .value("ONE", BlendFactor::ONE)
        .value("SRC_COLOR", BlendFactor::SRC_COLOR)
        .value("ONE_MINUS_SRC_COLOR", BlendFactor::ONE_MINUS_SRC_COLOR)
        .value("DST_COLOR", BlendFactor::DST_COLOR)
        .value("ONE_MINUS_DST_COLOR", BlendFactor::ONE_MINUS_DST_COLOR)
        .value("SRC_ALPHA", BlendFactor::SRC_ALPHA)
        .value("ONE_MINUS_SRC_ALPHA", BlendFactor::ONE_MINUS_SRC_ALPHA)
        .value("DST_ALPHA", BlendFactor::DST_ALPHA)
        .value("ONE_MINUS_DST_ALPHA", BlendFactor::ONE_MINUS_DST_ALPHA)
        .value("SRC_ALPHA_SATURATE", BlendFactor::SRC_ALPHA_SATURATE);

    py::enum_<CompositeOperation>(module, "CompositeOperation", py::arithmetic())
        .value("SOURCE_OVER", CompositeOperation::SOURCE_OVER)
        .value("SOURCE_IN", CompositeOperation::SOURCE_IN)
        .value("SOURCE_OUT", CompositeOperation::SOURCE_OUT)
        .value("ATOP", CompositeOperation::ATOP)
        .value("DESTINATION_OVER", CompositeOperation::DESTINATION_OVER)
        .value("DESTINATION_IN", CompositeOperation::DESTINATION_IN)
        .value("DESTINATION_OUT", CompositeOperation::DESTINATION_OUT)
        .value("DESTINATION_ATOP", CompositeOperation::DESTINATION_ATOP)
        .value("LIGHTER", CompositeOperation::LIGHTER)
        .value("COPY", CompositeOperation::COPY)
        .value("XOR", CompositeOperation::XOR);

    py::enum_<ImageFlags>(module, "ImageFlags", py::arithmetic())
        .value("GENERATE_MIPMAPS", ImageFlags::GENERATE_MIPMAPS)
        .value("REPEAT_X", ImageFlags::REPEAT_X)
        .value("REPEAT_Y", ImageFlags::REPEAT_Y)
        .value("FLIP_Y", ImageFlags::FLIP_Y)
        .value("PREMULTIPLIED", ImageFlags::PREMULTIPLIED)
        .value("NEAREST", ImageFlags::NEAREST);

    py::class_<Image, ImagePtr> Py_Image(module, "Image");
    Py_Image.def_property_readonly("size", &Image::get_size, DOCSTR("The size of the Image"));

    py::class_<Paint> Py_Paint(module, "Paint");
    Py_Paint.def_readwrite("xform", &Paint::xform, DOCSTR("The transformation of the Paint's coordinate system."));
    Py_Paint.def_readwrite("extent", &Paint::extent, DOCSTR("The 2D extent of the Paint."));
    Py_Paint.def_readwrite("radius", &Paint::radius, DOCSTR("Radius value used when drawing gradients."));
    Py_Paint.def_readwrite("feather", &Paint::feather, DOCSTR("Feater value used when drawing gradients."));
    Py_Paint.def_readwrite("inner_color", &Paint::inner_color, DOCSTR("Inner gradient color."));
    Py_Paint.def_readwrite("outer_color", &Paint::outer_color, DOCSTR("Outer gradient color."));
    Py_Paint.def_readwrite("image", &Paint::image, DOCSTR("Image to paint."));

    py::class_<Font> Py_Font(module, "Font");
    Py_Font.def("__bool__", [](const Font& font) -> bool { return font.id != -1; });

    py::class_<GlyphPosition> Py_GlyphPosition(module, "GlyphPosition");
    Py_GlyphPosition.def_readonly("index", &GlyphPosition::index, DOCSTR("Index in UTF-8 code points of the glyph in its string."));
    Py_GlyphPosition.def_readonly("x_advance", &GlyphPosition::x_advance, DOCSTR("Horizontal position of the logical glyph."));
    Py_GlyphPosition.def_readonly("x_min", &GlyphPosition::x_min, DOCSTR("Left bound of the glyph shape."));
    Py_GlyphPosition.def_readonly("x_max", &GlyphPosition::x_max, DOCSTR("Right bound of the glyph shape."));

    py::class_<TextRow> Py_TextRow(module, "TextRow");
    Py_TextRow.def_readonly("text", &TextRow::text, DOCSTR("Text contained in the row."));
    Py_TextRow.def_readonly("first_char_index", &TextRow::first_char_index, DOCSTR("Index of the first character in the row in the original string."));
    Py_TextRow.def_readonly("width", &TextRow::width, DOCSTR("Logical width of the row."));
    Py_TextRow.def_readonly("x_min", &TextRow::x_min, DOCSTR("Left bound of the row shape."));
    Py_TextRow.def_readonly("x_max", &TextRow::x_max, DOCSTR("Rightbound of the row shape."));

    py::class_<FontMetrics> Py_FontMetrics(module, "FontMetrics");
    Py_FontMetrics.def_readonly("ascender", &FontMetrics::ascender, DOCSTR("Font ascender value."));
    Py_FontMetrics.def_readonly("descender", &FontMetrics::descender, DOCSTR("Font descender value."));
    Py_FontMetrics.def_readonly("line_height", &FontMetrics::line_height, DOCSTR("Font line height."));

    // nanovg

    py::class_<NanoVG, std::shared_ptr<NanoVG>> Py_NanoVG(module, "NanoVG");
    Py_NanoVG.def(py::init<>());

    Py_NanoVG.def("begin_frame", &NanoVG::begin_frame, DOCSTR("Begin drawing a new frame."), py::arg("window_width"), py::arg("window_height"), py::arg("device_pixel_ratio") = 1.f);
    Py_NanoVG.def("cancel_frame", &NanoVG::cancel_frame, DOCSTR("Cancel drawing the current frame."));
    Py_NanoVG.def("end_frame", &NanoVG::end_frame, DOCSTR("Ends drawing, flushing the remaining render state."));

    Py_NanoVG.def("save", &NanoVG::save, DOCSTR("Pushes and saves the current render state into a state stack."));
    Py_NanoVG.def("restore", &NanoVG::restore, DOCSTR("Pops and restores current render state."));
    Py_NanoVG.def("reset", &NanoVG::reset, DOCSTR("Resets current render state to default values. Does not affect the render state stack."));

    Py_NanoVG.def("shape_anti_alias", &NanoVG::shape_anti_alias, DOCSTR("Sets whether to draw antialias for stroke() and fill()."), py::arg("enabled") = true);
    Py_NanoVG.def("stroke_color", &NanoVG::stroke_color, DOCSTR("Sets current stroke style to a solid color."), py::arg("color") = notf::Color(0, 0, 0, 255));
    Py_NanoVG.def("stroke_paint", &NanoVG::stroke_paint, DOCSTR("Sets current stroke style to a paint, which can be a one of the gradients or a pattern."), py::arg("paint"));
    Py_NanoVG.def("fill_color", &NanoVG::fill_color, DOCSTR("Sets current fill style to a solid color."), py::arg("color") = notf::Color(255, 255, 255, 255));
    Py_NanoVG.def("fill_paint", &NanoVG::fill_paint, DOCSTR("Sets current fill style to a paint, which can be a one of the gradients or a pattern."), py::arg("paint"));
    Py_NanoVG.def("miter_limit", &NanoVG::miter_limit, DOCSTR("Sets the miter limit of the stroke style."), py::arg("limit") = 10.f);
    Py_NanoVG.def("stroke_width", &NanoVG::stroke_width, DOCSTR("Sets the miter limit of the stroke style."), py::arg("width") = 1.f);
    Py_NanoVG.def("line_cap", &NanoVG::line_cap, DOCSTR("Sets how the end of the line (cap) is drawn."), py::arg("cap") = LineCap::BUTT);
    Py_NanoVG.def("line_join", &NanoVG::line_join, DOCSTR("Sets how sharp path corners are drawn."), py::arg("join") = LineCap::MITER);
    Py_NanoVG.def("global_alpha", &NanoVG::global_alpha, DOCSTR("Sets the transparency applied to all rendered shapes."), py::arg("alpha") = 1.);
    Py_NanoVG.def("global_composite_operation", &NanoVG::global_composite_operation, DOCSTR("Sets the composite operation."), py::arg("operation") = CompositeOperation::SOURCE_OVER);
    Py_NanoVG.def("global_composite_blend_func", &NanoVG::global_composite_blend_func, DOCSTR("Sets the composite operation with custom pixel arithmetic."), py::arg("src_factor"), py::arg("dst_factor"));
    Py_NanoVG.def("global_composite_blend_func_separate", &NanoVG::global_composite_blend_func_separate, DOCSTR("Sets the composite operation with custom pixel arithmetic for RGB and alpha components separately."), py::arg("src_rgb"), py::arg("dst_rgb"),py::arg("src_alpha"), py::arg("dst_alpha"));

    Py_NanoVG.def("reset_transform", &NanoVG::reset_transform, DOCSTR("Resets the current transform to a identity matrix."));
    Py_NanoVG.def("transform", &NanoVG::transform, DOCSTR("Premultiplies the current coordinate system by specified matrix."), py::arg("a") = 1.f, py::arg("b") = 0.f, py::arg("c") = 0.f, py::arg("d") = 1.f, py::arg("e") = 0.f, py::arg("f") = 0.f);
    Py_NanoVG.def("translate", &NanoVG::translate, DOCSTR("Translates the current coordinate system."), py::arg("x") = 0.f, py::arg("y") = 0.f);
    Py_NanoVG.def("rotate", &NanoVG::rotate, DOCSTR("Rotates current coordinate system. Angle is specified in radians."), py::arg("angle") = 0.f);
    Py_NanoVG.def("skew_x", &NanoVG::skew_x, DOCSTR("Skews the current coordinate system along X axism. Angle is specified in radians."), py::arg("angle") = 0.f);
    Py_NanoVG.def("skew_y", &NanoVG::skew_y, DOCSTR("Skews the current coordinate system along Y axis. Angle is specified in radians."), py::arg("angle") = 0.f);
    Py_NanoVG.def("scale", &NanoVG::scale, DOCSTR("Scales the current coordinate system."), py::arg("x") = 1.f, py::arg("y") = 1.f);
    Py_NanoVG.def("current_transform", &NanoVG::current_transform, DOCSTR("The current transformation matrix."));

    Py_NanoVG.def("create_image", &NanoVG::create_image, DOCSTR("Creates an image by loading it from the disk from specified file name."), py::arg("file_path"), py::arg("flags") = ImageFlags(0));

    Py_NanoVG.def("linear_gradient", &NanoVG::linear_gradient, DOCSTR("Creates a linear gradient."), py::arg("start_x"), py::arg("start_y"), py::arg("end_x"), py::arg("end_y"), py::arg("start_color"), py::arg("end_color"));
    Py_NanoVG.def("box_gradient", &NanoVG::box_gradient, DOCSTR("Creates a box gradient."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"), py::arg("radius"), py::arg("feather"), py::arg("start_color"), py::arg("end_color"));
    Py_NanoVG.def("radial_gradient", &NanoVG::radial_gradient, DOCSTR("Creates a radial gradient."), py::arg("center_x"), py::arg("center_y"), py::arg("inner_radius"), py::arg("outer_radius"), py::arg("start_color"), py::arg("end_color"));
    Py_NanoVG.def("image_pattern", &NanoVG::image_pattern, DOCSTR("Creates an image pattern."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"), py::arg("angle"), py::arg("image"), py::arg("alpha") = 1.f);

    Py_NanoVG.def("scissor", &NanoVG::scissor, DOCSTR("Sets the current scissor rectangle."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"));
    Py_NanoVG.def("intersect_scissor", &NanoVG::intersect_scissor, DOCSTR("Intersects current scissor rectangle with the specified rectangle."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"));
    Py_NanoVG.def("reset_scissor", &NanoVG::reset_scissor, DOCSTR("Reset and disables scissoring."));

    Py_NanoVG.def("begin_path", &NanoVG::begin_path, DOCSTR("Clears the current path and sub-paths."));
    Py_NanoVG.def("move_to", &NanoVG::move_to, DOCSTR("Starts new sub-path with specified point as first point."), py::arg("x"), py::arg("y"));
    Py_NanoVG.def("line_to", &NanoVG::line_to, DOCSTR("Adds line segment from the last point in the path to the specified point."), py::arg("x"), py::arg("y"));
    Py_NanoVG.def("bezier_to", &NanoVG::bezier_to, DOCSTR("Adds cubic bezier segment from last point in the path via two control points to the specified point."), py::arg("ctrl1_x"), py::arg("ctrl1_y"), py::arg("ctrl2_x"), py::arg("ctrl2_y"), py::arg("end_x"), py::arg("end_y"));
    Py_NanoVG.def("quad_to", &NanoVG::quad_to, DOCSTR("Adds quadratic bezier segment from last point in the path via a control point to the specified point."), py::arg("ctrl_x"), py::arg("ctrl_y"), py::arg("end_x"), py::arg("end_y"));
    Py_NanoVG.def("arc_to", &NanoVG::arc_to, DOCSTR("Adds an arc segment at the corner defined by the last path point, and two specified points."), py::arg("x1"), py::arg("y1"), py::arg("x2"), py::arg("y2"), py::arg("radius"));
    Py_NanoVG.def("close_path", &NanoVG::close_path, DOCSTR("Closes current sub-path with a line segment."));
    Py_NanoVG.def("path_winding", &NanoVG::path_winding, DOCSTR("Sets the current sub-path winding"), py::arg("winding"));
    Py_NanoVG.def("arc", &NanoVG::arc, DOCSTR("Creates new circle arc shaped sub-path."), py::arg("center_x"), py::arg("center_y"), py::arg("arc_radius"), py::arg("angle_start"), py::arg("angle_end"), py::arg("direction"));
    Py_NanoVG.def("rect", &NanoVG::rect, DOCSTR("Creates new rectangle shaped sub-path."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"));
    Py_NanoVG.def("rounded_rect", &NanoVG::rounded_rect, DOCSTR("Creates new rounded rectangle shaped sub-path."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"), py::arg("radius"));
    Py_NanoVG.def("rounded_rect_varying", &NanoVG::rounded_rect_varying, DOCSTR("Creates new rounded rectangle shaped sub-path with varying radii for each corner."), py::arg("left"), py::arg("top"), py::arg("width"), py::arg("height"), py::arg("radius_top_left"), py::arg("radius_top_right"), py::arg("radius_bottom_right"), py::arg("radius_bottom_left"));
    Py_NanoVG.def("ellipse", &NanoVG::ellipse, DOCSTR("Creates new ellipse shaped sub-path."), py::arg("center_x"), py::arg("center_y"), py::arg("radius_x"), py::arg("radius_y"));
    Py_NanoVG.def("circle", &NanoVG::circle, DOCSTR("Creates new circle shaped sub-path."), py::arg("center_x"), py::arg("center_y"), py::arg("radius"));
    Py_NanoVG.def("fill", &NanoVG::fill, DOCSTR("Fills the current path with current fill style."));
    Py_NanoVG.def("stroke", &NanoVG::stroke, DOCSTR("Strokes the current path with current stroke style."));

    Py_NanoVG.def("create_font", &NanoVG::create_font, DOCSTR("Creates font by loading it from the disk from specified file name."), py::arg("name"), py::arg("file_path"), py::arg("index") = -1);
    Py_NanoVG.def("find_font", &NanoVG::find_font, DOCSTR("Finds a loaded font of specified name."), py::arg("name"));
    Py_NanoVG.def("add_fallback_font", &NanoVG::add_fallback_font, DOCSTR("Adds a fallback font."), py::arg("base"), py::arg("fallback"));
    Py_NanoVG.def("reset_fallback_fonts", &NanoVG::reset_fallback_fonts, DOCSTR("Resets fallback fonts."), py::arg("base"));
    Py_NanoVG.def("font_size", &NanoVG::font_size, DOCSTR("Sets the font size of current text style."), py::arg("size") = 16.f);
    Py_NanoVG.def("font_blur", &NanoVG::font_blur, DOCSTR("Sets the blur of current text style."), py::arg("blur") = 0.f);
    Py_NanoVG.def("text_letter_spacing", &NanoVG::text_letter_spacing, DOCSTR("Sets the letter spacing of current text style."), py::arg("spacing") = 0.f);
    Py_NanoVG.def("text_line_height", &NanoVG::text_line_height, DOCSTR("Sets the proportional line height of current text style. The line height is specified as multiple of font size."), py::arg("line_height") = 1.f);
    Py_NanoVG.def("text_align", &NanoVG::text_align, DOCSTR("Sets the text align of current text style"), py::arg("align") = Align(Align::LEFT | Align::BASELINE));
    Py_NanoVG.def("font_face", &NanoVG::font_face, DOCSTR("Sets the font face of the current text style."), py::arg("font"));
    Py_NanoVG.def("text", &NanoVG::text, DOCSTR("Draws text string at specified location. Returns the horizontal advance."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("char_count") = -1);
    Py_NanoVG.def("text_box", &NanoVG::text_box, DOCSTR("Draws multi-line text string at specified location wrapped at the specified width."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("width"), py::arg("char_count") = -1);
    Py_NanoVG.def("text_bounds", &NanoVG::text_bounds, DOCSTR("Measures the specified text string. Returns a tuple (AABR, horizontal advance)."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("char_count") = -1);
    Py_NanoVG.def("text_box_bounds", &NanoVG::text_box_bounds, DOCSTR("Measures the specified multi-text string."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("width"), py::arg("char_count") = -1);
    Py_NanoVG.def("text_glyph_positions", &NanoVG::text_glyph_positions, DOCSTR("Calculates the glyph x positions of the specified text."), py::arg("x"), py::arg("y"), py::arg("string"), py::arg("char_count") = -1);
    Py_NanoVG.def("text_metrics", &NanoVG::text_metrics, DOCSTR("Returns the vertical metrics based on the current text style."));
    Py_NanoVG.def("text_break_lines", &NanoVG::text_break_lines, DOCSTR("Breaks the specified text into lines."), py::arg("string"), py::arg("width"), py::arg("char_count") = -1, py::arg("max_rows") = 100);
}
