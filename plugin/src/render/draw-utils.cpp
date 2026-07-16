#include "draw-utils.h"

#include <cmath>
#include <vector>

#include <obs.h>

#include <graphics/graphics.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>

namespace sk {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// The atlas is an 8-bit coverage mask; color/alpha are applied here per draw
// call rather than baked into the bitmap (see glyph-atlas.h).
const char* kTextEffectSource = R"(
uniform float4x4 ViewProj;
uniform texture2d image;
uniform float4 color;

sampler_state textSampler {
    Filter   = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VertInOut {
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VertInOut VSText(VertInOut vert_in)
{
    VertInOut vert_out;
    vert_out.pos = mul(float4(vert_in.pos.xyz, 1.0), ViewProj);
    vert_out.uv  = vert_in.uv;
    return vert_out;
}

float4 PSText(VertInOut vert_in) : TARGET
{
    float coverage = image.Sample(textSampler, vert_in.uv).r;
    return float4(color.rgb, color.a * coverage);
}

technique Draw
{
    pass
    {
        vertex_shader = VSText(vert_in);
        pixel_shader  = PSText(vert_in);
    }
}
)";

gs_effect_t* text_effect() {
    // Created once, lazily, on first use, and kept alive for the plugin's
    // lifetime -- all video_render calls happen sequentially on OBS's
    // graphics thread, so a function-local static is safe here and avoids
    // every source instance compiling its own copy of the same effect.
    static gs_effect_t* effect = [] {
        char* error = nullptr;
        gs_effect_t* e = gs_effect_create(kTextEffectSource, "screencast-keys-text.effect", &error);
        if (!e && error) {
            blog(LOG_ERROR, "[screencast-keys] text effect compile error: %s", error);
            bfree(error);
        }
        return e;
    }();
    return effect;
}

void set_solid_color(const Color& color) {
    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t* color_param = gs_effect_get_param_by_name(solid, "color");
    vec4 v{};
    vec4_set(&v, color.r, color.g, color.b, color.a);
    gs_effect_set_vec4(color_param, &v);
}

// Runs `emit_verts` (which should call gs_vertex2f, optionally paired with
// gs_texcoord, per vertex) inside a gs_render_start/gs_render_stop block.
// gs_render_stop(mode) performs the actual draw internally.
template <typename EmitFn>
void immediate_draw(gs_draw_mode mode, EmitFn&& emit_verts) {
    gs_render_start(true);
    emit_verts();
    gs_render_stop(mode);
}

void draw_solid_rect(float x, float y, float w, float h, const Color& color) {
    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");
    set_solid_color(color);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    immediate_draw(GS_TRISTRIP, [&] {
        gs_vertex2f(x, y);
        gs_vertex2f(x + w, y);
        gs_vertex2f(x, y + h);
        gs_vertex2f(x + w, y + h);
    });

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

// Thick line as a filled quad (perpendicular-offset rectangle), rather than
// relying on GPU line-width support (unsupported on most modern backends
// beyond 1px) or a custom polyline shader.
void draw_solid_line(float x1, float y1, float x2, float y2, float thickness, const Color& color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) {
        return;
    }
    dx /= len;
    dy /= len;
    const float half = thickness * 0.5f;
    const float nx = -dy * half;
    const float ny = dx * half;

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");
    set_solid_color(color);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    immediate_draw(GS_TRISTRIP, [&] {
        gs_vertex2f(x1 + nx, y1 + ny);
        gs_vertex2f(x1 - nx, y1 - ny);
        gs_vertex2f(x2 + nx, y2 + ny);
        gs_vertex2f(x2 - nx, y2 - ny);
    });

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

// Number of arc segments for a corner of the given radius, mirroring
// ops.py's circle_verts_num (aims for ~2px per segment, floored/ceiled to a
// reasonable range).
int arc_segments_for_radius(float radius) {
    int num_verts = 32;
    const float threshold = 2.0f;
    while (true) {
        if (radius * 2.0f * kPi / static_cast<float>(num_verts) > threshold) {
            return std::max(num_verts / 4, 1);
        }
        num_verts -= 4;
        if (num_verts < 1) {
            return 1;
        }
    }
}

// Builds the full outline of a rounded rect (going around all four
// corners), reused for both the filled (triangle-fan-from-center) and
// outline (line loop) cases. Direct port of ops.py's draw_rounded_box
// corner-arc generation.
std::vector<vec2> rounded_rect_outline(float x, float y, float w, float h, float radius,
                                        const RoundCorners& rc) {
    const float radii[4] = {
        rc.bottom_right ? radius : 0.0f,
        rc.bottom_left ? radius : 0.0f,
        rc.top_right ? radius : 0.0f,
        rc.top_left ? radius : 0.0f,
    };
    const float x_origin[4] = {x + radii[0], x + w - radii[1], x + w - radii[2], x + radii[3]};
    const float y_origin[4] = {y + radii[0], y + radii[1], y + h - radii[2], y + h - radii[3]};
    const float angle_start[4] = {kPi * 1.0f, kPi * 1.5f, kPi * 0.0f, kPi * 0.5f};

    std::vector<vec2> points;
    for (int corner = 0; corner < 4; ++corner) {
        const float r = radii[corner];
        const int n = arc_segments_for_radius(r > 0 ? r : radius) + 1;
        const float dangle = kPi * 2.0f / static_cast<float>(4 * (n - 1));
        float angle = angle_start[corner];
        for (int i = 0; i < n; ++i) {
            vec2 p{};
            p.x = x_origin[corner] + r * std::cos(angle);
            p.y = y_origin[corner] + r * std::sin(angle);
            points.push_back(p);
            angle += dangle;
        }
    }
    return points;
}

void draw_rounded_box(const DrawCommand& cmd) {
    std::vector<vec2> outline =
        rounded_rect_outline(cmd.x, cmd.y, cmd.w, cmd.h, cmd.radius, cmd.round_corners);
    if (outline.empty()) {
        return;
    }

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");
    set_solid_color(cmd.color);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    if (cmd.filled) {
        // Manual fan-to-triangles: (center, p[i], p[i+1]) for each edge,
        // matching GL_TRIANGLE_FAN semantics (libobs's gs_draw_mode has no
        // fan primitive).
        vec2 center{};
        center.x = cmd.x + cmd.w * 0.5f;
        center.y = cmd.y + cmd.h * 0.5f;

        immediate_draw(GS_TRIS, [&] {
            for (size_t i = 0; i + 1 < outline.size(); ++i) {
                gs_vertex2f(center.x, center.y);
                gs_vertex2f(outline[i].x, outline[i].y);
                gs_vertex2f(outline[i + 1].x, outline[i + 1].y);
            }
            // Closing wedge: without this, the fan never connects the last
            // outline point back to the first, leaving a wedge-shaped gap
            // cut into the shape (visible as a seam through the middle of
            // any filled rounded rect).
            gs_vertex2f(center.x, center.y);
            gs_vertex2f(outline.back().x, outline.back().y);
            gs_vertex2f(outline.front().x, outline.front().y);
        });
    } else {
        immediate_draw(GS_LINESTRIP, [&] {
            for (const vec2& p : outline) {
                gs_vertex2f(p.x, p.y);
            }
            gs_vertex2f(outline.front().x, outline.front().y); // Close the loop.
        });
    }

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

void draw_text(const DrawCommand& cmd, GlyphAtlas& atlas) {
    gs_effect_t* effect = text_effect();
    if (!effect || !atlas.texture()) {
        return;
    }

    gs_eparam_t* color_param = gs_effect_get_param_by_name(effect, "color");
    gs_eparam_t* image_param = gs_effect_get_param_by_name(effect, "image");
    gs_technique_t* tech = gs_effect_get_technique(effect, "Draw");

    vec4 color_v{};
    vec4_set(&color_v, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
    gs_effect_set_vec4(color_param, &color_v);
    gs_effect_set_texture(image_param, atlas.texture());

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    float pen_x = cmd.x;
    for (unsigned char c : cmd.text) {
        const GlyphInfo* glyph = atlas.find_glyph(cmd.font_size, c);
        if (!glyph) {
            continue;
        }
        if (glyph->width > 0 && glyph->height > 0) {
            const float qx = pen_x + static_cast<float>(glyph->bearing_x);
            const float qy = cmd.y - static_cast<float>(glyph->bearing_y);
            const float qw = static_cast<float>(glyph->width);
            const float qh = static_cast<float>(glyph->height);

            immediate_draw(GS_TRISTRIP, [&] {
                gs_texcoord(glyph->u0, glyph->v0, 0);
                gs_vertex2f(qx, qy);
                gs_texcoord(glyph->u1, glyph->v0, 0);
                gs_vertex2f(qx + qw, qy);
                gs_texcoord(glyph->u0, glyph->v1, 0);
                gs_vertex2f(qx, qy + qh);
                gs_texcoord(glyph->u1, glyph->v1, 0);
                gs_vertex2f(qx + qw, qy + qh);
            });
        }
        pen_x += glyph->advance;
    }

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

// Full-white-multiply textured blit (matches ops.py's draw_custom_mouse,
// which draws with immColor4f(1, 1, 1, 1) -- no color tint applied to
// custom user-supplied images, unlike text/shapes).
void draw_image(const DrawCommand& cmd, CustomMouseImages& mouse_images) {
    gs_texture_t* tex = mouse_images.texture(cmd.image_slot);
    if (!tex) {
        return;
    }

    gs_effect_t* default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    gs_eparam_t* image_param = gs_effect_get_param_by_name(default_effect, "image");
    gs_technique_t* tech = gs_effect_get_technique(default_effect, "Draw");

    gs_effect_set_texture(image_param, tex);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    immediate_draw(GS_TRISTRIP, [&] {
        gs_texcoord(0.0f, 0.0f, 0);
        gs_vertex2f(cmd.x, cmd.y);
        gs_texcoord(1.0f, 0.0f, 0);
        gs_vertex2f(cmd.x + cmd.w, cmd.y);
        gs_texcoord(0.0f, 1.0f, 0);
        gs_vertex2f(cmd.x, cmd.y + cmd.h);
        gs_texcoord(1.0f, 1.0f, 0);
        gs_vertex2f(cmd.x + cmd.w, cmd.y + cmd.h);
    });

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

} // namespace

void draw_display_list(const DisplayList& commands, GlyphAtlas& atlas, CustomMouseImages* mouse_images) {
    atlas.flush_pending_uploads();
    if (mouse_images) {
        mouse_images->flush_pending_uploads();
    }

    gs_enable_blending(true);
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    for (const DrawCommand& cmd : commands) {
        switch (cmd.type) {
            case DrawCommandType::Rect:
                draw_solid_rect(cmd.x, cmd.y, cmd.w, cmd.h, cmd.color);
                break;
            case DrawCommandType::RoundedRect:
                draw_rounded_box(cmd);
                break;
            case DrawCommandType::Line:
                draw_solid_line(cmd.x, cmd.y, cmd.x2, cmd.y2, cmd.line_thickness, cmd.color);
                break;
            case DrawCommandType::Text:
                draw_text(cmd, atlas);
                break;
            case DrawCommandType::Image:
                if (mouse_images) {
                    draw_image(cmd, *mouse_images);
                }
                break;
        }
    }
}

} // namespace sk
