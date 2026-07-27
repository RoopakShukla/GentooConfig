# bfetch

This is the microoptimized C version of `bfetch`.

The point was to see how much real system inspection could be crammed into a one-shot binary before the kernel, and filesystem metadata overhead finally told me to shut the fuck up.

So this version keeps same rules:

- no shelling out
- no daemon
- no caching layer pretending to be speed
- no fake per-distro detection shortcuts beyond art selection
- real system checks every run
- one buffered write at the end

That is the whole shit.

## What This File Actually Is

The C path is centered around [`fetch.c`](./fetch.c).

 It is a deliberately aggressive Linux userspace probe with:

- raw syscall wrappers
- direct `/proc` and `/sys` parsing
- `getdents64` directory walking
- `mmap` where large file scans are worth it
- inline hot paths
- one final buffered output write


## The Real Enemy Was Never Arithmetic

At first glance this kind of program looks like it should be "instant".

And arithmetic-wise, sure, it basically is.

The math for most fields is trivial:

- distro: read one small file and grab one string
- kernel: one `uname`
- uptime: parse seconds, do a few integer divisions
- memory: parse `MemTotal` and `MemAvailable`, subtract, scale to GiB
- CPU: three CPUID leaves, thread count parse, one frequency read

That part is not the wall.

The wall is metadata and kernel-facing book keeping:

- opening procfs and sysfs files
- walking package database directories
- scanning Nix manifests
- chasing parent processes for terminal detection
- resolving a pretty GPU model name instead of a generic vendor string

So the runtime is better described as:

```text
T_total = T_startup + T_syscalls + T_vfs + T_scan + T_format
```

Not:

```text
T_total = "C is fast so it should be free"
```

That distinction matters. A lot.

## What Got Optimized

The major pressure points were:

### 1. Output building

The renderer builds one buffer in memory and flushes it with a single `write`.

That matters because once the data is collected, multiple tiny writes are just self-inflicted syscall overhead.

### 2. Package counting

This was and still is one of the biggest exact-work terms.

If you insist on real counts with no caching, then the program has to pay for:

- pacman directory enumeration
- flatpak directory enumeration
- nix manifest scanning

 That is the actual cost of knowing the answer right now.

### 3. GPU naming

A generic GPU vendor string is cheap.

An actually nice GPU name is not.

This C version tries hard to keep the expensive part contained, but if you want `NVIDIA GeForce RTX 3070 Ti` instead of some miserable fallback, you are paying for extra procfs/sysfs and lookup work.

### 4. Tiny file reads

A lot of this program is "open a tiny virtual file, read a small buffer, parse it, close it".

None of those individual operations are huge, but when the whole binary is fighting for sub-millisecond execution, they matter as a class.


## What `fetch.c` Is Trying To Detect

The structure in [`fetch.c`](./fetch.c) is split by field on purpose so each detector can be reasoned about independently.

At a high level:

- distro
  - `/etc/os-release`
  - same read also drives art choice logic
- kernel
  - `uname`
- uptime
  - `/proc/uptime`
- memory
  - `/proc/meminfo`
- WM
  - environment
- terminal
  - env first, then parent process inspection
- shell
  - environment plus basename trim
- CPU
  - CPUID plus sysfs
- GPU
  - DRM or NVIDIA-specific live checks
- packages
  - native filesystem/database inspection, no subprocesses

That separation is not just code style. It makes benchmarking and blame assignment possible. If one field is expensive, you can isolate it and prove it instead of arguing from vibes.

## Why Keep This Version At All

Because its still the cleanest expression of the actual problem.

It lets the program sit very close to the machine:

- direct control over buffers
- direct control over syscalls
- direct control over parsing
- direct control over data layout
- almost no abstraction tax unless you choose to pay it

That does not mean every line of C is automatically faster than everything else. It means this is the version where the intended behavior, performance strategy, and Linux-facing tricks are the clearest.

Everything else in the repo is either learning from this version or fighting it.

## Performance Reality

The exact number moves around with system noise, current machine state, package database size, and what the kernel feels like doing.

But on this box, this version is already in the sub-millisecond class when benchmarked sanely with no shell startup in the way.

That is the important part.

Not "did it hit some magic ideological number every single run", but:

- it is doing real work
- it is doing that work natively
- it is doing it absurdly fast already

At this point, further wins tend to come from:

- reducing VFS work
- reducing package database traversal cost
- reducing GPU naming overhead
- reducing startup overhead

Not from pretending another clever `for` loop is going to save the world.

## Build

From the repo root:

```bash
make
```

Run it:

```bash
./fetch
./fetch --version
```

Benchmark it without shell noise:

```bash
hyperfine --shell=none ./fetch
```

## What This Version Proved

It proved that the problem is not "C is too slow".

It proved that once the code gets tight enough, the remaining fight is mostly with:

- kernel entry/exit overhead
- procfs/sysfs formatting
- directory enumeration
- manifest scanning
- process startup

Which is exactly the kind of answer you want if you are trying to understand where the real lower bound lives instead of posting compiler flag astrology.

So if you want the version that best represents the actual microoptimization war in this repo, this is it.
