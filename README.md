# Screencast Keys (OBS plugin)

<img width="125" height="102" alt="image" src="https://github.com/user-attachments/assets/5167ed8e-7947-40a2-b306-df7c1ea043aa" />

A native OBS Studio source plugin that shows pressed keys and mouse
buttons on stream/recording, ported from the
[Blender Screencast Keys addon](../screencast_keys) in this repo. It aims
to *behave* like the Blender addon (fading event history with repeat-count
collapsing, a held-modifier-key pill, a mouse hold-status icon, extensive
styling) rather than replicate [univrsal/input-overlay](https://github.com/univrsal/input-overlay)'s
static icon-atlas approach, while still using the same global-input-hook
technique (`libuiohook`) that input-overlay uses.

## Installation

You need a built `screencast-keys.dll` (Windows) plus this repo's `data/`
folder. If you don't have one yet, build it first -- see
[BUILDING.md](BUILDING.md) (there is no separate binary download; this
plugin links against a `libobs` built locally to match your installed OBS
version).

### Windows

OBS Studio (28+) automatically scans a per-machine plugin directory under
`%ProgramData%` in addition to its own installed `obs-plugins` folder --
this location is writable without administrator rights, which is the
easiest way to install or update the plugin:

1. Create this folder layout (if it doesn't already exist):
   ```
   C:\ProgramData\obs-studio\plugins\screencast-keys\bin\64bit\
   C:\ProgramData\obs-studio\plugins\screencast-keys\data\
   ```
2. Copy `screencast-keys.dll` into the `bin\64bit\` folder.
3. Copy this plugin's `data\` folder (the `locale\en-US.ini` file) into the
   `data\` folder above.
4. **Close OBS Studio first if it's running** -- it locks the DLL while
   loaded, so copying over it while OBS is open will fail with
   "Device or resource busy" / access denied.
5. Launch OBS Studio.

Alternatively, if you'd rather install system-wide (requires admin), copy
the same two things into your OBS installation instead:
```
<OBS install dir>\obs-plugins\64bit\screencast-keys.dll
<OBS install dir>\data\obs-plugins\screencast-keys\
```

### macOS / Linux

Not built/tested on these platforms yet (see BUILDING.md and the
architecture notes below -- the code is written to be portable, but only
the Windows build has actually been compiled and run). The equivalent
locations, if you build it yourself, are:
- macOS: `~/Library/Application Support/obs-studio/plugins/screencast-keys.plugin/`
- Linux: `~/.config/obs-studio/plugins/screencast-keys/`

## Adding the source in OBS

1. In OBS, click **+** under Sources and choose **Screencast Keys**.
2. Open its **Properties** to configure styling (color, shadow, background,
   font size, mouse icon size/position, text position/spacing, etc. -- see
   below).
3. The source's canvas auto-sizes to fit whatever's currently drawn; drag it
   in the scene like any other source to position/scale it on your canvas.

### Settings overview

The mouse icon, the held-modifier ("Ctrl"/"Shift"/...) pill, and the text
history are three **independent** elements -- none of them push or resize
each other:

| Setting | What it controls |
|---|---|
| Mouse Size X / Y | Size of the mouse icon (or custom image), always anchored at the source's top-left corner |
| Use Custom Mouse Image + Base/Left/Right/Middle Image | Swap the drawn mouse icon for your own images (Normal = swap on click, Overlay = stack on top of the base image) |
| Shortcut Offset X / Y | Where the held-modifier-keys pill is drawn |
| Text Initial Offset X / Y | Where the **newest** history line is drawn |
| Text Spacing X / Y | How far each **older** line is offset from the newest, per step -- negative values are valid and reverse the direction (e.g. a negative Y makes older lines climb upward instead of stacking downward) |
| Background / Background Mode / Background Color / Corner Radius / Margin | Draw a rounded-rect background behind either each text line ("Text" mode) or the whole canvas ("Draw Area" mode) |
| Color / Shadow / Shadow Color | Text and icon color, optional drop shadow |
| Font Size | Size used for all text (history lines and the modifier pill) |
| Display Time | Seconds a key press stays visible before fading out |
| Max Event History | Cap on the number of visible history lines |
| Repeat Count | Collapse repeated presses of the same key into "A x3" instead of one line per press |
| Show Mouse Events + Mode | Whether mouse clicks appear in the text history, the hold-status icon, both, or neither |

### Hotkeys

Settings > Hotkeys > **Enable Screencast Keys** / **Disable Screencast
Keys** -- pauses/resumes this source instance's capture and display
without affecting other Screencast Keys sources or the source's normal
OBS visibility toggle.

## What's different from the Blender addon (and why)

| Blender addon | This plugin |
|---|---|
| Sees only events routed to Blender's own viewport | Sees OS-wide keyboard/mouse events via a global hook -- the only sensible behavior for a stream overlay showing input to *other* windows |
| Single content block: mouse/modifiers at the bottom, history stacked above it | Three independent elements (mouse icon, modifier pill, text history), each with its own anchor -- avoids the mouse icon visibly shifting as history lines appear/expire |
| `origin` (Region/Area/Window/Cursor), `align`, fixed `offset`/`margin` | Dropped (Blender-editor-specific, or superseded by the independent-element model above). The canvas always auto-sizes to content; position the source on your OBS canvas via its normal scene-item transform |
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
    mouse-icon-images.{h,cpp}      gs_image_file2-based custom mouse image loading
tests/                             standalone unit tests for state/event-history, state/input-state,
                                   input/event-types -- zero OBS/libuiohook dependency
```

The hard architectural rule: **`gs_*` graphics calls only happen inside
`video_render`**. `video_tick` drains the input hook's mailbox, updates the
pure-logic state (`InputState`/`EventHistory`), and runs `layout.cpp` to
produce a `DisplayList` -- plain data, no texture handles. `draw-utils.cpp`
is the only file that turns that into actual GPU draw calls.
