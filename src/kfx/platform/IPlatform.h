#ifndef IPLATFORM_H
#define IPLATFORM_H

#include "bflib_basics.h" // TbBool
#include "bflib_sound.h"  // SoundVolume

class IWindowSystem;
class IFileSystem;
struct SDL_Window;
struct TbFileFind;
struct TbFileEntry;

/** Per-OS platform services. PlatformManager selects the concrete
 *  implementation (PlatformWindows / PlatformLinux) for the build target;
 *  engine code reaches it only through the PlatformManager_* facade. */
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // ----- OS information -----
    virtual const char* GetOSVersion() const = 0;
    virtual const void* GetImageBase() const = 0;
    virtual const char* GetWineVersion() const = 0; // nullptr when not running under Wine
    virtual const char* GetWineHost() const = 0;    // nullptr when not running under Wine

    virtual TbFileFind* FileFindFirst(const char* filespec, TbFileEntry* entry) = 0;

    /** Per-user preferences directory (SDL_GetPrefPath("keeperfx","keeperfx"),
     *  falling back to the platform's own data path if SDL can't provide
     *  one) -- where renderer_prefs.ini (RendererSettings) is read/written.
     *  Lazily resolved and cached; not const since the first call populates
     *  the cache. Returned pointer is valid for the lifetime of the
     *  platform object. */
    virtual const char* GetUserPrefDir() = 0;

    // ----- Redbook (CD) audio -----
    virtual void   SetRedbookVolume(SoundVolume vol) = 0;
    virtual TbBool PlayRedbookTrack(int track) = 0;
    virtual void   PauseRedbookTrack() = 0;
    virtual void   ResumeRedbookTrack() = 0;
    virtual void   StopRedbookTrack() = 0;

    // ----- Steam ----- (Linux stubbed; real libsteam_api.so is future work)
    virtual int  InitSteam() = 0;   // 0 ok, -1 unsupported, >0 failure
    virtual void ShutdownSteam() = 0;

    /** Initialise the display subsystem. Per-OS: Windows adjusts SDL hints
     *  before SDL_Init, Linux initialises plainly. Returns false on failure. */
    virtual bool VideoInit() = 0;

    /** True on consoles that own the display exclusively. Desktop: false. */
    virtual bool OwnsDisplay() const { return false; }

    /** True where all registered video modes are reported available without
     *  querying SDL (consoles that own the display). Desktop: false. */
    virtual bool ForcesAllModesAvailable() const { return false; }

    /** The window system backing this platform (SDL desktop backend). */
    virtual IWindowSystem* GetWindowSystem();

    /** Whole-file storage backend for saves and other small data files. The default wraps LbFile*. */
    virtual IFileSystem* GetFileSystem();

    /** Adjust a desktop-fullscreen window so the OS compositor keeps compositing
     *  it, rather than letting the driver present it straight to the display. */
    virtual void KeepFullscreenWindowComposited(SDL_Window* /*window*/) {}

    /** Log display and HDR details for the game window. */
    virtual void LogDisplayDiagnostics(SDL_Window* /*window*/) {}

    /** Whether the monitor showing a w x h (pixels) mode at `refresh_hz` is
     *  driven with a variable refresh rate right now (FreeSync, G-Sync,
     *  VESA Adaptive-Sync): 1 yes, 0 no, -1 unknown. SDL has no way to ask, so
     *  this is the desktop's own answer where the platform can get one. */
    virtual int DisplayHasVariableRefresh(int /*w*/, int /*h*/, float /*refresh_hz*/) { return -1; }

    /** Move `abs_path` into the OS's native trash/recycle bin, in a way the desktop
     *  shell can restore it (freedesktop Trash spec on Linux; the platform's own
     *  equivalent elsewhere). Returns false -- "not trashed" -- when the OS trash is
     *  unavailable or the move fails for any reason; the caller is expected to fall
     *  back to its own retained trash directory. A false return must never mean
     *  "deleted anyway": on failure the file at abs_path is left untouched.
     *
     *  Once a file lands in the OS trash it belongs to the user and to their
     *  desktop's own retention policy -- implementations must never purge, scan, or
     *  otherwise manage what is already there; they only ever put things in.
     *
     *  macOS: no implementation exists here (no Mac build to test one against). The
     *  correct call there is Cocoa's -[NSFileManager trashItemAtURL:resultingItemURL:
     *  error:], not a plain move into ~/.Trash -- moving a file in by hand does not
     *  register it with Finder, so "Put Back" stays greyed out. Faking recoverability
     *  with a raw move would look like it worked and then never restore correctly;
     *  better to leave the seam unimplemented than ship that. */
    virtual TbBool TrashFile(const char* abs_path) = 0;
};

/** The platform implementation selected for this build target. */
IPlatform* GetPlatform();

#endif // IPLATFORM_H
