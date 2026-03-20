# Contributing

Quick notes to get the project building and running on Debian/Ubuntu systems.

Developer setup (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y \
  build-essential g++ make pkg-config cmake git \
  libgtk-3-dev libgtksourceview-3.0-dev libglu1-mesa-dev libgl1-mesa-dev libepoxy-dev libglm-dev \
  libssl-dev libpng-dev libfreetype-dev libpango1.0-dev libatk1.0-dev \
  libatkmm-1.6-dev libpangomm-1.4-dev libsigc++-2.0-dev libglib2.0-dev libgdk-pixbuf2.0-dev libdbus-1-dev \
  libx11-dev libxi-dev libxrandr-dev libxext-dev libxrender-dev
```

Build

```bash
cd /path/to/ProGen3d-main
make
# binary: ./progen3d
```

Run (interactive)

If you have a desktop session available just run:

```bash
./progen3d
```

Headless / CI

To run headless (e.g., capture a render or test startup), use Xvfb:

```bash
sudo apt install -y xvfb
xvfb-run -s "-screen 0 1600x1200x24" ./progen3d
```

Runtime caveat: snap / glibc mismatch

On some machines a `snap` runtime can cause symbol lookup / GLIBC compatibility errors when launching the binary. Example:

```
./progen3d: symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0: undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
```

This happens because a snap-provided library is being picked up at runtime instead of the host's system library. Workarounds:

- Run the binary from a non-snap shell (avoid snap-provided environment). Launch a plain terminal outside snap wrappers.
- Run in a clean environment (example):

```bash
env -i DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY PATH=/usr/bin:/bin HOME=$HOME ./progen3d
```

- Use a container or chroot that matches the host glibc (recommended for reproducible builds/runs).

If you still hit problems, capture the exact error and `ldd ./progen3d` output and open an issue.

Quick debugging commands

```bash
# show dynamic dependencies
ldd ./progen3d

# run with minimal env to avoid snap libraries
env -i DISPLAY=$DISPLAY PATH=/usr/bin:/bin HOME=$HOME ./progen3d
```

Thank you — contributions welcome. If you update build flags or dependencies, please update this file and `.github/copilot-instructions.md`.
Developer setup and quick start

This file gives a reproducible developer setup for building and running ProGen3d on Debian/Ubuntu.

Install system dependencies (Debian/Ubuntu example):

```bash
sudo apt update
sudo apt install -y \
  build-essential g++ make pkg-config cmake git \
  libgtk-3-dev libgtksourceview-3.0-dev libglu1-mesa-dev libgl1-mesa-dev libepoxy-dev libglm-dev \
  libssl-dev libpng-dev libfreetype-dev libpango1.0-dev libatk1.0-dev \
  libatkmm-1.6-dev libpangomm-1.4-dev libsigc++-2.0-dev libglib2.0-dev libgdk-pixbuf2.0-dev libdbus-1-dev \
  libx11-dev libxi-dev libxrandr-dev libxext-dev libxrender-dev xvfb
```

Build:

```bash
cd /path/to/ProGen3d-main
make
```

Run (interactive):

```bash
./progen3d
```

Run (headless) — quick test with Xvfb (virtual framebuffer):

```bash
xvfb-run -s "-screen 0 1600x1200x24" ./progen3d 2>&1 | tee runtime.log
```

Runtime caveat

On some systems (for example when a `snap` runtime is present) you may get a symbol lookup / GLIBC compatibility error when launching the binary:

```
./progen3d: symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0: undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
```

If you see that error, run the binary from a native shell (outside any snap confinement) or build/run inside a matching chroot/container. The easiest workaround for local development is to run the binary in a container or native environment that matches your distribution's glibc.

Problems or PRs

- If you change parsing or tokens: include a short example input in `Grammar.cpp` and the expected `tokens_new` sequence.
- If you change GL rendering: include a note in `main.cpp` explaining any changes to the shader setup or texture binding.

Thanks — open a PR to update this doc if you find better reproducible steps for other distributions.
