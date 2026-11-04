#include "pre_inc.h"
#include "kfx/platform/PlatformLinux.h"
#include "kfx/platform/FileFind.h"
#include "platform.h" // kfxmain
#include "bflib_fileio.h"
#include "config.h" // keeper_runtime_directory (GetUserPrefDir() SDL-less fallback)
#include "bflib_datetm.h" // LbDateTime, TbDate, TbTime
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <climits> // PATH_MAX
#include <cerrno>
#include <algorithm>
#include <memory>
#include <regex>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <unistd.h>
#include "post_inc.h"

// Only the file-name part of a filespec is a pattern. The directory part is a
// path as configured -- an INSTALL_PATH such as "/games/DK [GOG]" names a real
// folder; it is not a bracket expression.
static const char* pattern_from_filespec(const char* filespec)
{
    const auto sep = strrchr(filespec, '/');
    return sep ? sep + 1 : filespec;
}

static bool filespec_is_pattern(const char* filespec)
{
    return strchr(pattern_from_filespec(filespec), '*') != nullptr;
}

static std::string directory_from_filespec(const char* filespec)
{
    const auto sep = strrchr(filespec, '/');
    if (sep && sep != filespec) {
        return std::string(filespec, sep - filespec);
    }
    return ".";
}

const char* PlatformLinux::GetOSVersion() const { return "Linux"; }
const void* PlatformLinux::GetImageBase() const { return nullptr; }
const char* PlatformLinux::GetWineVersion() const { return nullptr; } // running native
const char* PlatformLinux::GetWineHost() const { return nullptr; }    // running native

const char* PlatformLinux::GetUserPrefDir()
{
    static char pref_path[512] = {};
    if (pref_path[0] != '\0')
        return pref_path;
    char* sdl_path = SDL_GetPrefPath("keeperfx", "keeperfx");
    if (sdl_path)
    {
        snprintf(pref_path, sizeof(pref_path), "%s", sdl_path);
        size_t len = strlen(pref_path);
        if (len > 0 && pref_path[len - 1] == '/')
            pref_path[len - 1] = '\0';
        SDL_free(sdl_path);
    }
    else
    {
        // Fall back to the game's own runtime directory (where keeperfx.cfg
        // already lives) rather than an SDL-less user-pref concept this
        // branch doesn't otherwise have.
        snprintf(pref_path, sizeof(pref_path), "%s", keeper_runtime_directory);
    }
    return pref_path;
}

TbFileFind* PlatformLinux::FileFindFirst(const char* filespec, TbFileEntry* entry)
{
    try {
        auto ff = std::make_unique<TbFileFind>();
        bool is_pattern = filespec_is_pattern(filespec);
        const char* pattern = pattern_from_filespec(filespec);
        // Find the directory the way LbFileOpen finds a file: as written if it
        // exists, otherwise case-insensitively. Configs written on Windows
        // (LEVELS_LOCATION = levels/MyPack beside a real levels/mypack) name
        // folders in whatever case, and every file inside one already opens
        // through LbFileOpen -- only the listing failed, so the pack had "no
        // .LIF files" and was rejected.
        char dir_buf[PATH_MAX];
        const std::string path = LbFileCaseInsensitivePath(
            is_pattern ? directory_from_filespec(filespec).c_str() : filespec, dir_buf, sizeof(dir_buf));
        DIR* handle = opendir(path.c_str());
        if (handle) {
            while (auto de = readdir(handle)) {
                // Hidden entries are not the user's content on Unix, and
                // readdir() hands them over because the fnmatch() call below
                // omits FNM_PERIOD: a filespec of "*" or "*.cfg" matches
                // "._campaign.cfg" as readily as the real file. That matters
                // because campaigns, mods and workshop content reach a Linux
                // install as archives built on macOS or Windows, which
                // routinely carry AppleDouble sidecars ("._x.cfg", "._x.zip")
                // beside every real file. Enumerated, they get parsed as
                // campaigns, fed to the sprite loader as zips, counted as
                // levels, and reported on-screen as a mod to install -- and
                // since '.' sorts before ordinary letters, they land ahead of
                // the file they shadow, so any caller keying on position picks
                // the sidecar. build_music_index() already filters them for
                // music/; doing it here covers every caller instead.
                //
                // This subsumes the "." and ".." skips it replaces. Carried
                // across when upstream #5107 moved enumeration into the
                // platform layer; their version skips only "." and "..".
                if (de->d_name[0] == '.') {
                    continue;
                }
                if (is_pattern && fnmatch(pattern, de->d_name, FNM_CASEFOLD) != 0) {
                    continue;
                }
                const std::string file_path = path + "/" + de->d_name;
                struct stat sb;
                if (stat(file_path.c_str(), &sb) < 0 || !S_ISREG(sb.st_mode)) {
                    continue;
                }
                std::string key = de->d_name;
                for (size_t i = 0; i < key.size(); i++) {
                    key[i] = (char)tolower((unsigned char)key[i]);
                }
                ff->names.emplace_back(key, de->d_name);
            }
            closedir(handle);
        }
        if (!ff->names.empty()) {
            std::sort(ff->names.begin(), ff->names.end());
            entry->Filename = ff->names[0].second.c_str();
            return ff.release();
        }
    } catch (...) {}
    return nullptr;
}

/******************************************************************************/
// Trash: freedesktop.org Trash spec (files/ + info/*.trashinfo under
// $XDG_DATA_HOME/Trash). Writing Path= as the absolute original location is the
// entire point -- it is what lets the desktop's own file manager "Restore" put a
// deleted save straight back into save/ with no restore UI of our own.

// Percent-encode per the Trash spec: everything outside "A-Za-z0-9-_.~/" becomes
// %XX. '/' is left literal -- Path= is a full pathname, not one URI segment.
static std::string trash_percent_encode(const char* path)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (const unsigned char* p = (const unsigned char*)path; *p != '\0'; p++) {
        unsigned char c = *p;
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
                    c == '~' || c == '/';
        if (safe) {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0xF];
        }
    }
    return out;
}

// mkdir -p. Trash dirs are created 0700: only this user should ever read their
// own trash.
static bool trash_make_dir_recursive(const std::string& dir)
{
    if (dir.empty() || dir == "/")
        return true;
    struct stat sb;
    if (stat(dir.c_str(), &sb) == 0)
        return S_ISDIR(sb.st_mode);
    size_t slash = dir.find_last_of('/');
    if ((slash != std::string::npos) && (slash > 0)) {
        if (!trash_make_dir_recursive(dir.substr(0, slash)))
            return false;
    }
    if ((mkdir(dir.c_str(), 0700) != 0) && (errno != EEXIST))
        return false;
    return true;
}

// $XDG_DATA_HOME/Trash, defaulting to $HOME/.local/share/Trash per the base
// directory spec. Empty when neither variable gives us anywhere to put it.
static std::string trash_home_dir()
{
    const char* xdg = getenv("XDG_DATA_HOME");
    if ((xdg != nullptr) && (xdg[0] != '\0'))
        return std::string(xdg) + "/Trash";
    const char* home = getenv("HOME");
    if ((home == nullptr) || (home[0] == '\0'))
        return std::string();
    return std::string(home) + "/.local/share/Trash";
}

// Canonical absolute path for Path=. realpath() also resolves any "." / ".."
// components, which a relative keeper_runtime_directory can otherwise leave in
// place. Falls back to a plain cwd-join only if realpath() itself fails.
static std::string trash_to_absolute(const char* path)
{
    char resolved[PATH_MAX];
    if (realpath(path, resolved) != nullptr)
        return resolved;
    if (path[0] == '/')
        return path;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr)
        return path;
    return std::string(cwd) + "/" + path;
}

// rename() that fails with EEXIST rather than replace a file already at `to`.
// Where the kernel or filesystem lacks RENAME_NOREPLACE, a plain rename(): the
// caller has already picked a name that was free a moment ago.
static int trash_rename_noreplace(const char* from, const char* to)
{
#ifdef RENAME_NOREPLACE
    if (renameat2(AT_FDCWD, from, AT_FDCWD, to, RENAME_NOREPLACE) == 0)
        return 0;
    if ((errno != EINVAL) && (errno != ENOSYS))
        return -1;
#endif
    return rename(from, to);
}

TbBool PlatformLinux::TrashFile(const char* abs_path)
{
    // Under Flatpak, HOME is sandbox-redirected: a "Trash" written there is not the
    // host desktop's trash and no file manager will ever surface it, so writing one
    // would just be a second, invisible fallback trash. Decline immediately and let
    // the caller's own save/trash/ fallback take it instead.
    if (getenv("FLATPAK_ID") != nullptr)
        return false;

    std::string home = trash_home_dir();
    if (home.empty())
        return false;
    std::string files_dir = home + "/files";
    std::string info_dir  = home + "/info";
    if (!trash_make_dir_recursive(files_dir) || !trash_make_dir_recursive(info_dir))
        return false;

    // The game finds files case-insensitively (LbFileOpen); a save listed as
    // fx1g0003.sav may be FX1G0003.SAV on disk. Move -- and record in Path= --
    // the file that is really there, or realpath() and rename() both miss it.
    char disk_path[PATH_MAX];
    abs_path = LbFileCaseInsensitivePath(abs_path, disk_path, sizeof(disk_path));

    std::string full_path = trash_to_absolute(abs_path);
    const char* slash = strrchr(full_path.c_str(), '/');
    std::string basename = (slash != nullptr) ? (slash + 1) : full_path;
    std::string stem = basename;
    std::string ext;
    size_t dot = basename.find_last_of('.');
    if ((dot != std::string::npos) && (dot > 0)) {
        stem = basename.substr(0, dot);
        ext = basename.substr(dot);
    }

    // Reserve a unique trashinfo name first, atomically (O_EXCL), before touching
    // the save file itself: two deletes racing must not collide on one name, and if
    // anything below fails the original save is still untouched.
    std::string trash_name;
    std::string info_path;
    int info_fd = -1;
    for (int suffix = 0; suffix < 10000; suffix++) {
        trash_name = (suffix == 0) ? basename
            : (stem + "." + std::to_string(suffix + 1) + ext);
        // The name must be free in files/ too, not just in info/: a file can sit
        // there without its .trashinfo (a trasher that died between the two
        // steps, or one copied in by hand), and rename() would replace it.
        struct stat sb;
        if (lstat((files_dir + "/" + trash_name).c_str(), &sb) == 0)
            continue;
        if (errno != ENOENT)
            return false;
        info_path = info_dir + "/" + trash_name + ".trashinfo";
        info_fd = open(info_path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
        if (info_fd >= 0)
            break;
        if (errno != EEXIST)
            return false;
    }
    if (info_fd < 0)
        return false; // exhausted the suffix range; give up rather than clobber

    struct TbDate date;
    struct TbTime tm;
    LbDateTime(&date, &tm);
    char info_text[PATH_MAX + 128];
    int len = snprintf(info_text, sizeof(info_text),
        "[Trash Info]\nPath=%s\nDeletionDate=%04u-%02u-%02uT%02u:%02u:%02u\n",
        trash_percent_encode(full_path.c_str()).c_str(),
        (unsigned)date.Year, (unsigned)date.Month, (unsigned)date.Day,
        (unsigned)tm.Hour, (unsigned)tm.Minute, (unsigned)tm.Second);
    bool wrote_ok = (len > 0) && ((size_t)len < sizeof(info_text)) &&
        (write(info_fd, info_text, (size_t)len) == len);
    close(info_fd);
    if (!wrote_ok) {
        unlink(info_path.c_str());
        return false;
    }

    std::string dest = files_dir + "/" + trash_name;
    if (trash_rename_noreplace(abs_path, dest.c_str()) == 0)
        return true;

    if (errno == EXDEV) {
        // Trash lives on a different filesystem than the save -- rename() can't
        // cross that, so copy the bytes across and remove the original by hand.
        // "x": create dest exclusively, never truncate a file already there.
        FILE* src = fopen(abs_path, "rb");
        FILE* dst = (src != nullptr) ? fopen(dest.c_str(), "wbx") : nullptr;
        const bool created_dst = (dst != nullptr);
        bool copy_ok = false;
        if ((src != nullptr) && (dst != nullptr)) {
            char buf[65536];
            size_t n;
            copy_ok = true;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
                if (fwrite(buf, 1, n, dst) != n) {
                    copy_ok = false;
                    break;
                }
            }
            copy_ok = copy_ok && (ferror(src) == 0);
        }
        if (src != nullptr) fclose(src);
        if ((dst != nullptr) && (fclose(dst) != 0))
            copy_ok = false; // the final flush failed: the copy is incomplete
        if (copy_ok && (unlink(abs_path) == 0))
            return true;
        if (created_dst)
            unlink(dest.c_str()); // partial copy -- don't leave debris in the trash
    }
    unlink(info_path.c_str());
    return false;
}

bool PlatformLinux::VideoInit()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        return false;
    atexit(SDL_Quit);
    return true;
}

void   PlatformLinux::SetRedbookVolume(SoundVolume) {}
TbBool PlatformLinux::PlayRedbookTrack(int) { return false; }
void   PlatformLinux::PauseRedbookTrack() {}
void   PlatformLinux::ResumeRedbookTrack() {}
void   PlatformLinux::StopRedbookTrack() {}

int  PlatformLinux::InitSteam() { return -1; }
void PlatformLinux::ShutdownSteam() {}

/******************************************************************************/
// Process entry point.

extern "C" int main(int argc, char *argv[])
{
    return kfxmain(argc, argv);
}

/******************************************************************************/
// Variable refresh rate. SDL reports a mode's refresh rate but not whether the
// monitor is running it fixed or variable, and the frame-rate cap needs to know:
// on a fixed-refresh monitor a frame that misses a refresh is held for a whole
// extra one, on a variable-refresh monitor it is shown as soon as it is ready.
//
// GNOME (Mutter) says so in DisplayConfig.GetCurrentState: the current mode of a
// monitor carries 'refresh-rate-mode': <'variable'> when variable refresh is on.
// gdbus ships with GLib on every GNOME desktop; asking it once per display change
// costs a few milliseconds and saves linking a D-Bus library for one read-only
// call. Any failure (another desktop, no gdbus, a sandbox without access to the
// session bus) is "unknown", and the caller treats unknown as fixed refresh --
// the behaviour every earlier build had.
int PlatformLinux::DisplayHasVariableRefresh(int w, int h, float refresh_hz)
{
    // The game's own LD_LIBRARY_PATH (an AppImage's bundled libraries) must not
    // reach a system binary.
    FILE* pipe = popen("env -u LD_LIBRARY_PATH -u LD_PRELOAD gdbus call --session --timeout 2"
                       " --dest org.gnome.Mutter.DisplayConfig --object-path /org/gnome/Mutter/DisplayConfig"
                       " --method org.gnome.Mutter.DisplayConfig.GetCurrentState 2>/dev/null", "r");
    if (pipe == nullptr)
        return -1;
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0 && out.size() < (1u << 20))
        out.append(buf, n);
    pclose(pipe);
    if (out.empty())
        return -1;
    // One tuple per mode: ('3440x1440@143.923', 3440, 1440, 143.92..., 1.0, [scales], {props}).
    // Monitors without a variable mode never list 'refresh-rate-mode' at all.
    static const std::regex mode_re(R"(\('[^']*', (\d+), (\d+), ([0-9.]+), [0-9.]+, \[[^\]]*\], \{([^}]*)\}\))");
    int matched = -1;
    for (auto it = std::sregex_iterator(out.begin(), out.end(), mode_re); it != std::sregex_iterator(); ++it)
    {
        const std::smatch& m = *it;
        const std::string props = m[4].str();
        if (props.find("'is-current': <true>") == std::string::npos)
            continue;
        const int variable = (props.find("'refresh-rate-mode': <'variable'>") != std::string::npos) ? 1 : 0;
        const int mw = atoi(m[1].str().c_str());
        const int mh = atoi(m[2].str().c_str());
        const float mhz = (float)atof(m[3].str().c_str());
        if (mw == w && mh == h && mhz > refresh_hz - 0.5f && mhz < refresh_hz + 0.5f)
        {
            // Two identical monitors that disagree cannot be told apart from here.
            if (matched >= 0 && matched != variable)
                return -1;
            matched = variable;
        }
    }
    // Only a monitor whose current mode is the game display's (size in pixels and rate)
    // answers. A game shown inside a nested compositor (gamescope in a GNOME session)
    // is not on the desktop's monitor at all, and its display matches none of them.
    return matched;
}
