# bfetchaust

This is the Austral rewrite of `fetch.c`.

The goal here was not to make some cute alternate version. The goal was to get as close as possible to the real `bfetch` behavior, output, and style while keeping the fetch logic in Austral and rebuilding the missing low-level pieces ourselves.

Right now this version matches the C binary on the tested paths for:

- default output
- `--help`
- `--version`
- `--gentoo`
- `--bedrock`
- `--cachyos`

It also keeps the same buffered output model. Everything is assembled in memory and written out in one shot.

## What This Directory Is

`bfetchaust/` is its own little subproject.

The high-level split is:

- `src/Fetch.aum`
  - argument parsing
  - mode selection
  - exact fetch rendering
  - exact help and version output
- `runtime/BfetchAust.Posix.aui` and `runtime/BfetchAust.Posix.aum`
  - the raw OS boundary
  - file, dir, env, uname, process, and libc imports that Austral does not ship with
- `runtime/BfetchAust.FastIO.aui` and `runtime/BfetchAust.FastIO.aum`
  - byte buffer code
  - buffered writes
  - fast file reads
  - a few small formatting helpers
- `runtime/BfetchAust.Sys.aui` and `runtime/BfetchAust.Sys.aum`
  - all the actual probes for distro, kernel, uptime, WM, terminal, shell, packages, CPU, GPU, and memory
- `bfetchaust.c`
  - generated C output from the Austral compiler
  - not hand-written, not something you are meant to edit

## Getting Austral

Austral lives here:

- <https://github.com/austral/austral/>

This repo currently assumes you have that compiler checked out next to `bfetch` in `../austral` and that you built `austral.exe` there. If your compiler lives somewhere else, edit `AUSTRAL_BIN` in `bfetchaust/Makefile`.

## Build

From the repo root:

```bash
eval $(opam env --switch=default --set-switch)
make -C bfetchaust
```

Typecheck only:

```bash
eval $(opam env --switch=default --set-switch)
make -C bfetchaust tc
```

Run it:

```bash
./bfetchaust/bfetchaust
./bfetchaust/bfetchaust --help
./bfetchaust/bfetchaust --cachyos
```

Install it somewhere on your path if you want:

```bash
install -Dm755 bfetchaust/bfetchaust ~/.local/bin/bfetchaust
```

Or system-wide:

```bash
sudo install -Dm755 bfetchaust/bfetchaust /usr/local/bin/bfetchaust
```

## Why This Was More Annoying Than The C Version

The C version is just allowed to be direct.

It opens files, reads directories, uses `mmap`, pulls env vars, parses `/proc`, walks `/sys/class/drm`, pokes `cpuid`, and writes the final buffer. That is the whole point of `fetch.c`. It is a very small binary with basically no ceremony between the idea and the machine.

Austral is not like that.

Austral gives you strong rules about ownership and mutation, which is good, but it does not give you the Linux-facing standard library surface this project needs. So if you want to write a real system tool in it, you have to build that surface first.

That meant doing all of this before the port was even interesting:

- file reads for things like `/etc/os-release`, `/proc/cpuinfo`, `/proc/meminfo`, `/proc/uptime`
- directory walking for package managers
- environment access for `TERM`, `TERM_PROGRAM`, `SHELL`, `XDG_CURRENT_DESKTOP`, `DESKTOP_SESSION`
- process inspection for parent PID and terminal detection
- sysfs probing for DRM devices and GPU IDs
- uname access for kernel release
- one-buffer output helpers so the renderer could stay close to the C version

So the actual rewrite was not just "port fetch.c". It was "first build the missing chunk of a tiny Linux stdlib, then port fetch.c on top of it which nearly killed me".

## The Rules I Kept On This Port

I did not want this thing cheating, no.

So the rules were:

- no shelling out, same as fetch.c
- no subprocess-based probing
- no fake wrappers that hide the real work
- keep the real fetch logic in Austral
- match the C output as closely as possible instead of settling for "close enough"

The one place where reality still matters is the OS boundary. Austral without that boundary cannot even open a file. So the low-level imports live in `runtime/BfetchAust.Posix.*`, and the rest of the actual program logic stays above that in Austral.

## What Had To Change Compared To `fetch.c`

Some parts ported cleanly. Some absolutely did not.

### 1. The runtime had to be built first

In C, `fetch.c` just calls libc and the kernel-facing interfaces it needs.

In Austral, I had to build:

- a byte buffer type for all formatted output and small string work
- file read helpers
- pointer-based path helpers
- directory iteration wrappers
- env wrappers
- formatting helpers for decimal and hex output
- probe helpers for `/proc` and `/sys`

Without that, there is no port.

### 2. The first raw file I/O path was a dead end

The initial raw `open` path looked like the obvious thing to do. In practice it was the wrong path in this build, so the file runtime got rewritten around `fopen`, `fread`, and `fclose` instead. That made the I/O layer stable enough to keep moving.

### 3. Terminal detection had to change too

The simple symlink-based idea was not reliable enough here. Reading `/proc/<pid>/exe` was not the right fit for the actual process chain in this shit.

So terminal detection was switched to `/proc/<pid>/comm` plus `/proc/<pid>/stat`, walking the parent process chain until it found the terminal instead of the shell wrapper sitting in front of it.

### 4. Package counting stayed native

The point of `bfetch` is that it does not shell out, so the Austral version could not either.

That means package counting is still done natively:

- Pacman by directory counting
- Flatpak by filesystem inspection
- Snap by filesystem inspection
- DPKG by directory counting
- Nix by direct manifest scanning
- Gentoo by walking `var/db/pkg`

The Nix path was especially annoying because the original C version leans on `mmap` substring search. The Austral runtime ended up doing deterministic byte scanning over the manifest files yah now so it can keep up.

### 5. GPU naming had to copy the ugly shi too

Getting the vendor and device IDs is easy enough.

Matching the C output exactly is where the pain starts. The port had to reproduce the `pci.ids` lookup rules, the bracket stripping, and the vendor prefix heuristic so `NVIDIA GeForce RTX 3070 Ti` came out the same way instead of almost the same way, still don't know about vender revisions.

### 6. Help text even had to match the weird parts

The C binary prints `Usage: ./bfetch [OPTIONS]`.

That is not `argv[0]`. That is just what it prints.

So the Austral version prints the same literal string because the point here was parity, not doing what looks cleaner.

## Austral Itself

Austral is interesting because it is not trying to be Rust, and it is definitely not C with a nicer parser.

The big thing it cares about is linearity and regions.

That changes how you write code:

- mutation is explicit
- borrowing is explicit
- ownership mistakes get caught early
- branch cleanup has to be structurally consistent
- you cannot casually pass mutable state around and hope for shit to be sweet

That part is good.

The rough part is that you feel the lack of batteries constantly.

In Rust, if you want to build a tool like this, the language is strict but the ecosystem is huge. You have crates, helpers, wrappers, and a mature library story.

In C, nothing stops you, so you can go straight at the machine and be done fast if you know what you are doing.

Austral sits in a weird middle spot:

- stricter than C by a lot
- smaller and lower-level than Rust in practice
- much less ergonomic than mainstream memory-safe languages
- very little stdlib for real Linux systems work

So writing this felt like doing systems programming with one hand tied behind my back and another in my ass, but in a way that was still interesting because the language rules actually force discipline, and im a little kinkly like that.

## The Kind Of Pain Austral Adds

These were the repeated gotchas during the rewrite:

- hex literals and some surface syntax that look fine in other languages were not accepted the way I first wrote them
- chained boolean expressions sometimes had to be flattened into explicit nested control flow
- region mismatches show up fast if one helper accidentally ties input and output to the same region when they should not be
- linear cleanup rules mean a casual early return can turn into a type error if one branch consumes a value and another does not
- a lot of simple C string work becomes explicit byte-buffer work

That sounds like complaining, but it is really the shape of the language. Austral wants you to make ownership and mutation obvious. If you do not, it pushes back.

## What `bfetchaust.c` Tells You About How Austral Works

The generated C file is useful if you want to understand what Austral is actually doing.

A few things jump out to me:

- the runtime support is right at the top
- options and other small algebraic types get monomorphized into generated structs and tags
- foreign imports become plain `extern` declarations in the generated C
- the final output is a lot bigger and more mechanical than the source Austral code

So if you open `bfetchaust.c`, what you see is not "nice hand-written C". You see the compiler lowering Austral's model into a big C translation unit.

That file also made one performance issue obvious. Austral's default `bin` build path was invoking `cc` without real optimization flags, which was a bad fit for this project. That is why the `Makefile` here does not use the plain default binary mode anymore. It emits C first, then compiles that C with:

```bash
cc -O3 -march=native -fwrapv -Wno-builtin-declaration-mismatch
```

That one change cut a lot of the gap in peformance.

## Performance

This was never going to beat the C version without a fight.

Current rough numbers on my machine after switching to the optimized generated-C build:

- `./bfetch`: about `1.9 ms`
- `./bfetchaust/bfetchaust`: about `3.2 ms`

So the Austral version is still slower.

That matters, and I am not pretending otherwise and its unacceptable i know.

The good news is that the big stupid gap from the unoptimized default build path is gone. The bad news is that there is still real overhead left after that.

## Current State

Right now the port is in a solid state(like an nvme):

- output parity is there on the tested paths
- no shelling out
- fetch logic is in Austral
- runtime support is split into sane modules
- the build is stable
- the generated C path is compiled with actual optimization flags

What is still not perfect:

- it is still slower than the C binary
- the runtime layer exists because Austral does not ship the Linux-facing stdlib this shi needs
- there is still more room to chase performance if I want to push harder on `mmap` and other C-style shortcuts in the runtime

## If You Want To Read The Port

Start here:

- `src/Fetch.aum`
- `runtime/BfetchAust.FastIO.aum`
- `runtime/BfetchAust.Sys.aum`
- `runtime/BfetchAust.Posix.aui`

That gets you the whole picture fast.
