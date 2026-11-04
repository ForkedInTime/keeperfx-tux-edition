/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file WindowSystemSDL.cpp
 *     SDL3 desktop window-system implementation of IWindowSystem.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "kfx/platform/WindowSystemSDL.h"
#include "kfx/platform/IPlatform.h"
#include "kfx/platform/GLContextSDL.h"
#include "bflib_basics.h"
#include "bflib_video.h"
#include "bflib_fileio.h" // LbFileCaseInsensitivePath (window icon)
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "post_inc.h"

static void ApplyWindowIcon(SDL_Window *window)
{
    // Attach the icon to the window itself. This is what X11 taskbars read; on
    // Wayland it is ignored in favour of the app_id set before SDL_Init (see
    // LbScreenInitialize), so both are needed to cover the two display servers.
    // Non-fatal: a missing icon must never stop the game starting.
    //
    // Loaded from the data tree rather than compiled in: the Linux makefile has
    // no CMake step to embed it, and fxdata/ ships with every install anyway.
    // Cached so repeated mode changes do not re-read the file from disk.
    static SDL_Surface *icon = nullptr;
    if (icon == nullptr) {
        char icon_path[2048];
        icon = IMG_Load(LbFileCaseInsensitivePath("fxdata/keeperfx_icon.png", icon_path, sizeof(icon_path)));
        if (icon == nullptr) {
            WARNLOG("Could not load window icon 'fxdata/keeperfx_icon.png': %s", SDL_GetError());
            return;
        }
    }
    if (!SDL_SetWindowIcon(window, icon)) {
        WARNLOG("Could not set window icon: %s", SDL_GetError());
    }
}

/******************************************************************************/

static WindowSystemSDL s_sdlWindowSystem;

WindowSystemSDL* GetSDLWindowSystem()
{
    return &s_sdlWindowSystem;
}

// Translate a 0-based display index (as used by the span-all-displays loop in
// bflib_video.c) into an SDL3 display ID. Falls back to the primary display.
static SDL_DisplayID display_index_to_id(int index)
{
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    SDL_DisplayID id = (displays && index >= 0 && index < count)
                       ? displays[index]
                       : SDL_GetPrimaryDisplay();
    if (displays) SDL_free(displays);
    return id;
}

bool WindowSystemSDL::IsAppActive() const { return m_appActive; }
void WindowSystemSDL::OnFocusGained()     { m_appActive = true;  ApplyOsCursorPolicy(); }
void WindowSystemSDL::OnFocusLost()       { m_appActive = false; ApplyOsCursorPolicy(); }

void WindowSystemSDL::ApplyOsCursorPolicy()
{
    if (!m_window)
        return;
    const bool focused = (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    if (focused)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
}

void WindowSystemSDL::SetCursorGrab(bool grab)
{
    if (!m_window)
        return;
    // Relative mouse mode is a setting now (#5134), defaulting on, which is what
    // this fork's earlier hard revert of this function was protecting: on Wayland
    // relative mode IS the pointer-lock mechanism -- warping a confined pointer is
    // unreliable there, and grab alone leaves absolute positioning, which is not
    // what the camera code expects. A user who wants the grab+warp behaviour can
    // now ask for it, so the revert is gone.
    if (m_useRelativeMouse) {
        // deltas straight from the OS.
        SDL_SetWindowRelativeMouseMode(m_window, grab);
    }
    else{
        // confine the cursor to the window and re-center it.
        SDL_SetWindowMouseGrab(m_window, grab);
        if (grab)
            RecenterCursor();
    }
    ApplyOsCursorPolicy();
}

void WindowSystemSDL::SetUseRelativeMouse(bool relative)
{
    if (relative == m_useRelativeMouse)
        return;

    if (m_window)
    {
        if (m_useRelativeMouse)
            SDL_SetWindowRelativeMouseMode(m_window, false);
        else
            SDL_SetWindowMouseGrab(m_window, false);
    }
    m_useRelativeMouse = relative;
}

void WindowSystemSDL::SetCursorVisible(bool visible)
{
    if (visible)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

void WindowSystemSDL::WarpCursor(int x, int y)
{
    if (!m_window)
        return;
    int px = 0, py = 0, pw = 0, ph = 0;
    GetPresentRect(&px, &py, &pw, &ph);
    float ux = 1.0f, uy = 1.0f;
    GetPixelsPerWindowUnit(&ux, &uy);
    float wx = (float)x;
    float wy = (float)y;
    if (m_gameW > 0 && m_gameH > 0 && pw > 0 && ph > 0)
    {
        // Aim at the middle of the game pixel so reading it back lands on the same pixel.
        wx = (px + (x + 0.5f) * pw / m_gameW) / ux;
        wy = (py + (y + 0.5f) * ph / m_gameH) / uy;
    }
    SDL_WarpMouseInWindow(m_window, wx, wy);
}

void WindowSystemSDL::RecenterCursor()
{
    if (!m_window)
        return;
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    SDL_WarpMouseInWindow(m_window, w / 2.0f, h / 2.0f);
}

bool WindowSystemSDL::GetCursorPosition(int* out_x, int* out_y) const
{
    if (!m_window)
        return false;
    float wx = 0.0f, wy = 0.0f;
    SDL_GetMouseState(&wx, &wy);
    int px = 0, py = 0, pw = 0, ph = 0;
    GetPresentRect(&px, &py, &pw, &ph);
    float ux = 1.0f, uy = 1.0f;
    GetPixelsPerWindowUnit(&ux, &uy);
    int gx = (int)wx;
    int gy = (int)wy;
    if (m_gameW > 0 && m_gameH > 0 && pw > 0 && ph > 0)
    {
        gx = (int)SDL_floor((wx * ux - px) * m_gameW / pw);
        gy = (int)SDL_floor((wy * uy - py) * m_gameH / ph);
    }
    if (out_x) *out_x = gx;
    if (out_y) *out_y = gy;
    return true;
}

void WindowSystemSDL::GetCursorScale(float* out_sx, float* out_sy) const
{
    float sx = 1.0f, sy = 1.0f;
    int px = 0, py = 0, pw = 0, ph = 0;
    GetPresentRect(&px, &py, &pw, &ph);
    if (m_window && m_gameW > 0 && m_gameH > 0 && pw > 0 && ph > 0)
    {
        float ux = 1.0f, uy = 1.0f;
        GetPixelsPerWindowUnit(&ux, &uy);
        sx = ux * m_gameW / pw;
        sy = uy * m_gameH / ph;
    }
    if (out_sx) *out_sx = sx;
    if (out_sy) *out_sy = sy;
}

bool WindowSystemSDL::IsCursorInWindow() const
{
    if (!m_window)
        return false;
    // Fork: mouse focus, not global cursor position against the window rect.
    //
    // SDL_GetGlobalMouseState returns 0,0 on Wayland -- measured on this fork's
    // reference desktop, natively and through XWayland, with the same probe
    // returning exact coordinates under X11, so it is the platform and not the
    // measurement. Upstream's arithmetic then reports "cursor outside" for every
    // window that does not cover the screen origin, which in a windowed session
    // silently drops every ungrabbed motion event and disables edge panning.
    // Their Linux CI only compiles the engine, so nothing there would catch it.
    //
    // Mouse focus is delivered by enter/leave events instead of a position
    // query, so it needs no global coordinates and is correct on Wayland, X11
    // and Windows alike. It is what IsMouseInsideWindow() used before #5109.
    return SDL_GetMouseFocus() == m_window;
}

// ----- Window management -----

bool WindowSystemSDL::HasWindow() const          { return m_window != nullptr; }
SDL_Window* WindowSystemSDL::GetSDLWindow() const { return m_window; }

void WindowSystemSDL::SyncWindow()
{
    if (m_window)
        SDL_SyncWindow(m_window);
}

unsigned int WindowSystemSDL::GetWindowFlags() const
{
    if (!m_window)
        return 0;
    SDL_WindowFlags sdl_flags = SDL_GetWindowFlags(m_window);
    unsigned int kfx_flags = 0;
    if (sdl_flags & SDL_WINDOW_FULLSCREEN)
    {
        // SDL3: a NULL fullscreen mode means desktop (borderless) fullscreen;
        // a non-NULL mode means an exclusive video mode.
        if (SDL_GetWindowFullscreenMode(m_window) == nullptr)
            kfx_flags |= KFX_WF_FULLSCREEN_DESKTOP;
        else
            kfx_flags |= KFX_WF_FULLSCREEN_EXCLUSIVE;
    }
    if (sdl_flags & SDL_WINDOW_BORDERLESS) kfx_flags |= KFX_WF_BORDERLESS;
    if (sdl_flags & SDL_WINDOW_HIDDEN)     kfx_flags |= KFX_WF_HIDDEN;
    return kfx_flags;
}

void WindowSystemSDL::GetWindowSize(int* out_w, int* out_h) const
{
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (m_window == nullptr)
        return;
    SDL_GetWindowSize(m_window, out_w, out_h);
}

void WindowSystemSDL::GetDrawableSize(int* out_w, int* out_h) const
{
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (m_window == nullptr)
        return;
    SDL_GetWindowSizeInPixels(m_window, out_w, out_h);
}

void WindowSystemSDL::SetGameSurfaceSize(int w, int h)
{
    m_gameW = w;
    m_gameH = h;
}

void WindowSystemSDL::GetPresentRect(int* out_x, int* out_y, int* out_w, int* out_h) const
{
    int x = 0, y = 0, w = 0, h = 0;
    GetVisibleArea(&x, &y, &w, &h);
    if (m_gameW > 0 && m_gameH > 0 && w > 0 && h > 0)
    {
        const double scale = SDL_min((double)w / m_gameW, (double)h / m_gameH);
        const int fit_w = (int)(m_gameW * scale + 0.5);
        const int fit_h = (int)(m_gameH * scale + 0.5);
        x += (w - fit_w) / 2;
        y += (h - fit_h) / 2;
        w = fit_w;
        h = fit_h;
    }
    if (out_x) *out_x = x;
    if (out_y) *out_y = y;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
}

// Drawable pixels per window coordinate unit (cursor positions are in window units).
void WindowSystemSDL::GetPixelsPerWindowUnit(float* out_sx, float* out_sy) const
{
    float sx = 1.0f, sy = 1.0f;
    if (m_window != nullptr)
    {
        int ww = 0, wh = 0, dw = 0, dh = 0;
        SDL_GetWindowSize(m_window, &ww, &wh);
        SDL_GetWindowSizeInPixels(m_window, &dw, &dh);
        if (ww > 0 && wh > 0 && dw > 0 && dh > 0)
        {
            sx = (float)dw / (float)ww;
            sy = (float)dh / (float)wh;
        }
    }
    if (out_sx) *out_sx = sx;
    if (out_sy) *out_sy = sy;
}

// On-screen part of the drawable, in pixels from its top-left corner.
void WindowSystemSDL::GetVisibleArea(int* out_x, int* out_y, int* out_w, int* out_h) const
{
    int dw = 0, dh = 0;
    GetDrawableSize(&dw, &dh);
    int x = 0, y = 0, w = dw, h = dh;
    if (m_window != nullptr && (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN))
    {
        SDL_Rect win = {0, 0, 0, 0};
        SDL_Rect bounds = {0, 0, 0, 0};
        SDL_Rect visible = {0, 0, 0, 0};
        SDL_GetWindowPosition(m_window, &win.x, &win.y);
        SDL_GetWindowSize(m_window, &win.w, &win.h);
        if (win.w > 0 && win.h > 0
            && SDL_GetDisplayBounds(SDL_GetDisplayForWindow(m_window), &bounds)
            && SDL_GetRectIntersection(&win, &bounds, &visible))
        {
            // Window coordinates may be scaled points; convert to drawable pixels.
            const float sx = (float)dw / (float)win.w;
            const float sy = (float)dh / (float)win.h;
            x = (int)((visible.x - win.x) * sx + 0.5f);
            y = (int)((visible.y - win.y) * sy + 0.5f);
            w = (int)(visible.w * sx + 0.5f);
            h = (int)(visible.h * sy + 0.5f);
        }
    }
    if (out_x) *out_x = x;
    if (out_y) *out_y = y;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
}

int WindowSystemSDL::GetWindowDisplayIndex() const
{
    if (m_window)
        return (int)SDL_GetDisplayForWindow(m_window);

    return (int)SDL_GetPrimaryDisplay();
}

int WindowSystemSDL::GetNumVideoDisplays() const
{
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (displays)
        SDL_free(displays);
    return count;
}

int WindowSystemSDL::GetDesktopDisplayMode(int display, int* out_w, int* out_h) const
{
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    // SDL3 display IDs start at 1; 0 means "not set yet" — fall back to primary.
    SDL_DisplayID disp_id = (display > 0) ? (SDL_DisplayID)display : SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(disp_id);
    if (!mode)
        return -1;
    if (out_w) *out_w = mode->w;
    if (out_h) *out_h = mode->h;
    return 0;
}

int WindowSystemSDL::GetDisplayBounds(int display, int* out_x, int* out_y, int* out_w, int* out_h) const
{
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    SDL_DisplayID disp_id = display_index_to_id(display);
    SDL_Rect rect = {0, 0, 0, 0};
    if (!SDL_GetDisplayBounds(disp_id, &rect))
        return -1;
    if (out_x) *out_x = rect.x;
    if (out_y) *out_y = rect.y;
    if (out_w) *out_w = rect.w;
    if (out_h) *out_h = rect.h;
    return 0;
}

int WindowSystemSDL::GetClosestDisplayMode(int display, int desired_w, int desired_h, int* out_w, int* out_h) const
{
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    SDL_DisplayID disp_id = (display > 0) ? (SDL_DisplayID)display : SDL_GetPrimaryDisplay();
    SDL_DisplayMode closest = {};
    if (!SDL_GetClosestFullscreenDisplayMode(disp_id, desired_w, desired_h, 0.0f, false, &closest))
        return 0;
    if (out_w) *out_w = closest.w;
    if (out_h) *out_h = closest.h;
    return 1;
}

int WindowSystemSDL::SetWindowDisplayMode(int w, int h)
{
    if (m_window == nullptr)
        return -1;
    if (m_desktopFullscreenOnly)
    {
        // The renderer scales the game frame into a desktop fullscreen window instead.
        return SDL_SetWindowFullscreenMode(m_window, nullptr) ? 0 : -1;
    }
    SDL_DisplayID disp_id = SDL_GetDisplayForWindow(m_window);
    SDL_DisplayMode dm = {};
    if (SDL_GetClosestFullscreenDisplayMode(disp_id, w, h, 0.0f, false, &dm))
        return SDL_SetWindowFullscreenMode(m_window, &dm) ? 0 : -1;
    // No matching exclusive mode — fall back to desktop (borderless) fullscreen.
    return SDL_SetWindowFullscreenMode(m_window, nullptr) ? 0 : -1;
}

void WindowSystemSDL::SetWindowSize(int w, int h)
{
    if (m_window != nullptr)
        SDL_SetWindowSize(m_window, w, h);
}

int WindowSystemSDL::SetWindowFullscreen(unsigned int flags)
{
    if (!m_window)
        return -1;
    if (flags == 0)
        return SDL_SetWindowFullscreen(m_window, false) ? 0 : -1; // windowed
    if (flags & KFX_WF_FULLSCREEN_DESKTOP)
    {
        // Desktop (borderless) fullscreen at the native resolution.
        SDL_SetWindowFullscreenMode(m_window, nullptr);
        int result = SDL_SetWindowFullscreen(m_window, true) ? 0 : -1;
        if (result == 0)
            KeepFullscreenComposited();
        return result;
    }
    // Exclusive fullscreen: the specific mode is applied via SetWindowDisplayMode();
    // here we just enter fullscreen.
    int result = SDL_SetWindowFullscreen(m_window, true) ? 0 : -1;
    if (result == 0)
        KeepFullscreenComposited();
    return result;
}

void WindowSystemSDL::SetWindowBordered(int bordered)
{
    if (m_window != nullptr)
        SDL_SetWindowBordered(m_window, bordered ? true : false);
}

void WindowSystemSDL::SetWindowPosition(int x, int y)
{
    if (m_window != nullptr)
        SDL_SetWindowPosition(m_window, x, y);
}

bool WindowSystemSDL::CreateWindow(const char* title, int x, int y, int w, int h, unsigned int flags)
{
    // Translate KfxWindowFlags to SDL3 window creation flags.
    SDL_WindowFlags sdl3_flags = 0;
    if (flags & (KFX_WF_FULLSCREEN_EXCLUSIVE | KFX_WF_FULLSCREEN_DESKTOP))
        sdl3_flags |= SDL_WINDOW_FULLSCREEN;
    if (flags & KFX_WF_BORDERLESS) sdl3_flags |= SDL_WINDOW_BORDERLESS;
    if (flags & KFX_WF_HIDDEN)     sdl3_flags |= SDL_WINDOW_HIDDEN;

    if (flags & KFX_WF_OPENGL)
    {
        sdl3_flags |= SDL_WINDOW_OPENGL;
        // Created hidden regardless of the caller's KFX_WF_HIDDEN request:
        // SDL3 sets the window's pixel format as part of SDL_CreateWindow,
        // and doing that on a visible window can trigger a DWM
        // composition-pipeline reconfiguration (a visible black flash on
        // HDR displays). RendererOpenGL::Init() shows the window once the
        // GL context is fully created and current -- matches develop's
        // platform_create_gl_context() sequencing.
        sdl3_flags |= SDL_WINDOW_HIDDEN;
        GLContextSDL::RequestWindowAttributes();
    }

    m_window = SDL_CreateWindow(title, w, h, sdl3_flags);
    if (!m_window)
        return false;
    m_keepComposited = (flags & KFX_WF_KEEP_COMPOSITED) != 0;
    m_desktopFullscreenOnly = (flags & KFX_WF_DESKTOP_FULLSCREEN_ONLY) != 0;
    ApplyWindowIcon(m_window);
    GetPlatform()->LogDisplayDiagnostics(m_window);
    KeepFullscreenComposited();

    // A window created with SDL_WINDOW_FULLSCREEN starts as desktop fullscreen,
    // which is exactly what a desktop-fullscreen mode wants; an exclusive mode is
    // applied later via SetWindowDisplayMode(). Windowed modes get their position.
    if (!(sdl3_flags & SDL_WINDOW_FULLSCREEN))
        SDL_SetWindowPosition(m_window, x, y);
    return true;
}

void WindowSystemSDL::ShowWindow()
{
    if (m_window != nullptr)
        SDL_ShowWindow(m_window);
}

std::unique_ptr<IGLContext> WindowSystemSDL::CreateGLContext()
{
    return GLContextSDL::Create(m_window);
}

bool WindowSystemSDL::SetWindowTitle(const char* title)
{
    if (!m_window)
        return false;
    SDL_SetWindowTitle(m_window, title);
    return true;
}

void WindowSystemSDL::KeepFullscreenComposited()
{
    if (!m_keepComposited || m_window == nullptr)
        return;
    // Only desktop fullscreen: resizing an exclusive-mode window would fight the mode change.
    if ((SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) && SDL_GetWindowFullscreenMode(m_window) == nullptr)
        GetPlatform()->KeepFullscreenWindowComposited(m_window);
}

void WindowSystemSDL::HandleWindowEvent(const SDL_Event* ev)
{
    switch (ev->type)
    {
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        KeepFullscreenComposited();
        break;
    default:
        break;
    }
}

int WindowSystemSDL::GetDisplayVariableRefresh()
{
    if (m_window == nullptr)
        return -1;
    SDL_DisplayID disp_id = SDL_GetDisplayForWindow(m_window);
    const SDL_DisplayMode* mode = (disp_id != 0) ? SDL_GetCurrentDisplayMode(disp_id) : nullptr;
    if (mode == nullptr || mode->refresh_rate <= 0.0f)
        return -1;
    // The desktop lists modes in pixels; SDL's w/h are in points on a scaled display.
    const float density = (mode->pixel_density > 0.0f) ? mode->pixel_density : 1.0f;
    const int w = (int)(mode->w * density + 0.5f);
    const int h = (int)(mode->h * density + 0.5f);
    if (m_vrr_cached && m_vrr_display == disp_id && m_vrr_w == w && m_vrr_h == h && m_vrr_hz == mode->refresh_rate)
        return m_vrr_answer;
    m_vrr_answer = GetPlatform()->DisplayHasVariableRefresh(w, h, mode->refresh_rate);
    m_vrr_display = disp_id;
    m_vrr_w = w;
    m_vrr_h = h;
    m_vrr_hz = mode->refresh_rate;
    m_vrr_cached = true;
    return m_vrr_answer;
}

int WindowSystemSDL::GetDisplayRefreshRate() const
{
    if (m_window == nullptr)
        return 0;
    SDL_DisplayID disp_id = SDL_GetDisplayForWindow(m_window);
    if (disp_id == 0)
        return 0;
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(disp_id);
    if (mode && mode->refresh_rate > 0)
        return (int)(mode->refresh_rate + 0.5f);
    return 0;
}
