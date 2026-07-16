# Screencast Keys (OBS plugin)

A native OBS Studio source plugin that shows pressed keys and mouse
buttons on stream/recording, ported from the
[Blender Screencast Keys addon](../screencast_keys) in this repo. It aims
to *behave* like the Blender addon (fading event history with repeat-count
collapsing, a held-modifier-key pill, a mouse hold-status icon, extensive
styling) rather than replicate [univrsal/input-overlay](https://github.com/univrsal/input-overlay)'s
static icon-atlas approach, while still using the same global-input-hook
technique (`libuiohook`) that input-overlay uses.

## What's different from the Blender addon (and why)

| Blender addon | This plugin |
|---|---|
| Sees only events routed to Blender's own viewport | Sees OS-wide keyboard/mouse events via a global hook -- the only sensible behavior for a stream overlay showing input to *other* windows |
| `origin` (Region/Area/Window/Cursor) | Dropped (Blender-editor-specific). Replaced by `Canvas Size`: **Auto** (source bounding box tracks content, like the addon; position it via OBS's own scene-item transform) or **Fixed** (set an explicit canvas size; Align/Offset/Margin position the content inside it) |
| "Last Operator" display | Dropped -- no OBS analog |
| Per-key display-text alias editor | Dropped for now -- the underlying Left/Right-collapsing and per-OS modifier renaming (Ctrl/Alt/Win vs Control/Option/Command) is ported, just not user-editable per key yet |
| Auto-save / debug-log toggles | Dropped -- Blender-specific |

## Building

See [BUILDING.md](BUILDING.md).

## Source layout

```
src/
  plugin-main.cpp                 obs_module_load/unload, registers the source
  screencast-keys-source.cpp      obs_source_info glue (thin, delegates below)
  settings.{h,cpp}                RenderSettings struct + obs_properties_t UI
  input/
    event-types.{h,cpp}           libuiohook keycode -> our EventType -> display name
    global-input-hook.{h,cpp}     thin libuiohook wrapper (install/uninstall a global hook)
    input-hook-manager.{h,cpp}    refcounted singleton fanning the one hook out to N source instances
    raw-input-event.h             plain-data event handed from the hook thread to video_tick
  state/
    input-state.{h,cpp}           held modifiers (L/R-aware) + mouse button hold status
    event-history.{h,cpp}         fading, repeat-collapsing event history (pure logic, unit-tested)
  render/
    display-list.h                plain-data DrawCommand list -- the video_tick/video_render boundary
    layout.{h,cpp}                port of draw_area_size()/_draw_*_layer() -- produces a DisplayList
    glyph-atlas.{h,cpp}            FreeType glyph rasterization + atlas texture
    draw-utils.{h,cpp}             the only file issuing gs_* draw calls
    mouse-icon-images.{h,cpp}     gs_image_file2-based custom mouse image loading
tests/                             standalone unit tests for state/event-history, state/input-state,
                                   input/event-types -- zero OBS/libuiohook dependency, see tests/README
```

The hard architectural rule: **`gs_*` graphics calls only happen inside
`video_render`**. `video_tick` drains the input hook's mailbox, updates the
pure-logic state (`InputState`/`EventHistory`), and runs `layout.cpp` to
produce a `DisplayList` -- plain data, no texture handles. `draw-utils.cpp`
is the only file that turns that into actual GPU draw calls.
