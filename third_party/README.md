# libmpv

ChromaEngine embeds [libmpv](https://mpv.io/) to decode and render both MP4
video and GIF files through the same code path.

## Linux

Distro packages already include libmpv's headers and a `pkg-config` file -
`render/CMakeLists.txt` finds it via `pkg_check_modules(MPV REQUIRED
IMPORTED_TARGET mpv)`, no manual download or `-DMPV_DIR` needed. Install the
dev package for your distro before configuring the build:

- Debian/Ubuntu: `libmpv-dev`
- Fedora: `mpv-libs-devel`
- Arch: `mpv` (the runtime package already ships `mpv.pc` and the headers)

## Windows

There's no vcpkg port worth using here (it rebuilds ffmpeg from source and
takes a very long time) — use a prebuilt dev package instead:

1. Download a "dev" build from the community Windows builds:
   https://sourceforge.net/projects/mpv-player-windows/files/libmpv/
   Pick the `mpv-dev-x86_64-*.7z` archive (not `mpv-x86_64` — that one is the
   CLI player, not the dev package with headers/import lib).
2. Extract it somewhere, e.g. `C:\libs\mpv-dev`. As of the current builds you'll
   get:
   - `include/mpv/client.h`
   - `libmpv-2.dll` (note: newer builds dropped the old `mpv-2.dll` name)
   - `libmpv.dll.a` — a MinGW import lib, **not usable directly with MSVC's
     linker**. If you're building with MSVC (not MinGW), generate an MSVC
     `mpv.lib` from the DLL's own exports instead (verified working):
     ```
     # from a "Developer Command Prompt for VS 2022":
     dumpbin /exports libmpv-2.dll > exports.txt
     # build a .def listing just the public mpv_* API (the DLL also exports
     # unrelated bundled-library symbols you don't want):
     echo LIBRARY libmpv-2.dll> mpv.def
     echo EXPORTS>> mpv.def
     findstr /R "mpv_" exports.txt >> mpv.def   REM then trim down to just names
     lib /def:mpv.def /out:mpv.lib /machine:x64
     ```
     Place the resulting `mpv.lib` in the same directory as `libmpv-2.dll`
     and `include/`.
3. Configure the project with:
   ```
   cmake -B build -DMPV_DIR=C:\libs\mpv-dev -DCMAKE_PREFIX_PATH=<path to Qt6>
   ```

`mpv-2.dll` is copied next to the built executable automatically by
`platform/windows/CMakeLists.txt`.

## License

libmpv is LGPLv2.1+. It's dynamically linked (`mpv-2.dll` on Windows,
`libmpv.so` on Linux — loaded at runtime, not statically compiled in), which
keeps this project free to use its own license regardless of libmpv's. On
Windows, ship `mpv-2.dll` alongside the executable; on Linux, the distro's
own `libmpv`/`libmpv2` package satisfies this at install time instead, so it
belongs in the package's dependency list rather than being bundled. Either
way, keep a copy of mpv's license/notices with the distribution.
