## Quick orientation — what this repo is

ProGen3d is a small C++ OpenGL/GTK application that procedurally generates simple 3D scenes from a domain-specific grammar (.grammar files). The main pieces you'll interact with are:

- `main.cpp` — application entry, GTK UI, texture loading, GL shader setup and render loop. It orchestrates reading a grammar and initializing the `Grammar` and `Context` pipeline.
- `Grammar.*` / `grammar.h` — parser and interpreter for the small grammar language. Produces a linear list of executable `Token` objects (`tokens_new`) via `Recurse()`.
- `Context.*` / `Scope.*` — runtime scene/context stack. `Token::performAction(Context*)` mutates `Context` by pushing/popping `Scope`s, adding Primitives, or changing transform state.
- `Mesh.*`, `PLYWriter.*` — lightweight mesh/primitive helpers and optional PLY export.

Files to consult for behavior examples: `test.grammar`, `floor.grammar`, `groundscape.grammar` (grammar pattern usage) and `README.md` (high-level usage and examples).

## Short contract for changes

- Inputs: edits to `.grammar` files or C++ source.
- Outputs: same UI binary (`progen3d`) and generated geometry (in-memory Mesh or PLY files via `Context::PLY`).
- Error modes: parser/tokenization errors surface to GTK text buffer via `errorout()`; shader compile/link errors printed at GL init.

## Developer workflows — build, run, debug

- Build: this is a plain Makefile C++ project. Primary target builds `progen3d` with `g++` and many system-level includes/libs declared in `Makefile`.
- Typical commands (run from project root):

  - make
  - make clean

- Notes: the Makefile assumes system-wide development headers (GTK3, GL, glm, OpenCV, OpenSSL, etc.). If `make` fails with missing headers, install the corresponding -dev packages or edit `CXXFLAGS`/`LIBDYN`.

Developer setup (Debian/Ubuntu apt example)

  The fastest way to get a buildable environment on Debian/Ubuntu is to install the -dev packages below (run as sudo):

  ```bash
  sudo apt update
  sudo apt install -y \
    build-essential g++ make pkg-config cmake git \
    libgtk-3-dev libglu1-mesa-dev libgl1-mesa-dev libepoxy-dev libglm-dev \
    libopencv-dev libssl-dev libpng-dev libfreetype-dev libpango1.0-dev libatk1.0-dev \
    libatkmm-1.6-dev libpangomm-1.4-dev libsigc++-2.0-dev libglib2.0-dev libgdk-pixbuf2.0-dev libdbus-1-dev \
    libx11-dev libxi-dev libxrandr-dev libxext-dev libxrender-dev
  ```

  After installing deps, run `make` from the project root. If `make` succeeds you will get the `progen3d` binary.

Runtime caveat

  On some systems the runtime environment (for example when a `snap` runtime is present) can cause symbol lookup / GLIBC compatibility errors when launching the binary, e.g.:

  ```text
  ./progen3d: symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0: undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
  ```

  If you see that error, it means a different C library is being picked up at runtime (snap/<chroot> vs host). Run the binary in a native shell (outside `snap` confinement) or build/run inside a matching chroot/container to avoid mismatched glibc issues.

  Quick workaround (avoid snap/IDE-injected libs): run the program with a minimal environment which prevents snap wrappers from injecting alternate libs:

  ```bash
  env -i PATH=/usr/bin:/bin HOME=$HOME ./progen3d
  ```

  Headless tests: `xvfb-run` is a useful way to exercise rendering without a real display. On CI you may need to start Xvfb or use a runner that provides a display server.

- Debugging: run the compiled `./progen3d` under the debugger. Useful files to inspect when debugging: `main.cpp` (UI & GL init), `Grammar.cpp` (parser + Recurse), `Context.cpp` (primitive generation and PLY export).

## Project-specific patterns and conventions

- Single-pass grammar -> token list -> execution: `Grammar` reads `.grammar` lines, builds `Rule` objects, then `Recurse(rule_list[0])` flattens into `tokens_new`. Modifications should respect this 2-stage design (parse then execute).
- Variables are global-like and managed through `variable_list` and `full_variable_list` (see `Context.cpp`/`Grammar.cpp`). Many grammar constructs depend on side-effect variable push/pop ordering — be conservative when renaming or changing variable handling.
- Tokens are small command objects with `token_name`, `arguments`, `var_names[]`. The executor `Token::performAction(Context*)` implements semantics (`S`, `T`, `I`, `R`, `A`, `[` , `]`, etc.). If you add a new token, update parsing in `Grammar::ReadTokens`/`ReadTokens2` and add corresponding behavior in `performAction()`.
- Primitive instances use `Mesh::getInstance("Cube"|"Sphere"|"Cylinder")` and are applied into `Context::scene` via `Context::addPrimitive` and `Mesh::apply()`.

## Integration points & external dependencies

- Textures: loaded through `stb_image` (see `stbimage/stb_image.h`) in `main.cpp`, then `Context::loadTexture()` attaches them to context.
- OpenGL/GLSL: shader sources live inside `main.cpp`; shader compilation/logging occurs at `create_shader()` and `glLinkProgram()` — modify carefully, errors print to GTK warnings.
- Optional network/upload: `main.cpp` contains OpenSSL socket code gated by `loggedin` — unrelated to core generation; remove or stub for PRs that shouldn't touch networking.
- Export: `Context::PLY()` and `PLYWriter` provide binary PLY export paths — tests that rely on this should call `Context::PLY()` using the `calc()` vertex buffer.

## Concrete examples to reference in edits

- To add a new grammar token X(arg1,arg2):
  - update `Grammar::ReadTokens` to parse `X` and populate Token fields (see how `I`, `S`, and `R` are parsed).
  - implement behavior in `Token::performAction(Context*)` (follow `I`/`S` pattern).

- To change how textures are bound in GL rendering, examine `draw_buffer()` and `draw_box()` in `main.cpp` (they bind `texture_list[tex_index]` and set uniforms).

## What to avoid / gotchas

- Don't change the global `variable_list` semantics lightly — many grammar rules rely on value replacement side-effects (`MathS`, `MathS2` performing replacement/removal).
- The code mixes immediate UI side-effects (GTK) with core logic. Prefer non-UI refactors to keep UI calls in `main.cpp` and core logic in `Grammar/Context` files.
- The Makefile includes many absolute/static library references (LIBSA/LIBDYN). Tests on other machines will likely require editing `CXXFLAGS` and `LIBDYN` rather than changing the Makefile shape.

## Minimal PR checklist for code changes

- Build locally with `make` and run `./progen3d` to sanity check UI starts and a default grammar renders.
- Add a small grammar example in `test.grammar` and confirm geometry appears.
- If you touch parsing, include a short comment in `Grammar.cpp` showing an input example and expected token sequence.

---
If any section above is unclear or you want more detail (examples of token parsing, a short diagram of the execution flow, or automated build notes for a packaging target), tell me which part to expand and I will iterate.