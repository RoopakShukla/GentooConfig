# bfetchasm

This is the static assembly rewrite of `bfetch`.

 written in x86_64 assembly, doing live system checks on this machine and printing the same overall layout as the C version.

The reason this exists is simple: after enough microoptimizing in C, the obvious next question becomes "fine, what if I remove the language abstraction almost entirely and just talk to Linux myself?"

So this directory is that answer.

## What This Rewrite Was Trying To Prove

The assembly port was not made to disrespect the C version.

It was made to answer a harsher question:

If the real target is "as fast as physics allows on this Linux box", how much of the remaining cost is:

- C itself
- ELF / loader overhead
- libc-adjacent calling patterns
- extra abstraction in helpers

and how much is simply the unavoidable cost of asking the kernel real questions?

That is why the ASM version is:

- syscall-only
- statically linked
- machine-targeted
- still doing real detection
- still avoiding caching and daemons

## Where It Lives

The assembly implementation lives in [`Bfetchasm/fetch.s`](./Bfetchasm/fetch.s).

Its build is intentionally tiny and lives in [`Bfetchasm/Makefile`](./Bfetchasm/Makefile).

The code is split by detector in the same broad spirit as the C version:

- env scan and startup helpers
- file and directory I/O helpers
- string and number helpers
- field detectors for distro, kernel, uptime, memory, WM, terminal, shell, CPU, GPU, packages
- final buffered renderer

That separation matters because assembly gets unreadable very fast if every concern is just collapsed into one enormous blob of instructions.

## Why This Was Not Automatically Faster

This is the first thing worth being honest about.

Assembly is not magic.

If the bottleneck is:

- `open`
- `read`
- `getdents64`
- `readlinkat`
- procfs/sysfs formatting
- walking package metadata

then rewriting the surrounding glue in assembly does not erase that cost.

The math is still:

```text
T_total = T_exec + T_kernel_boundary + T_metadata + T_scan + T_render
```

And for a fetch tool doing exact live checks, `T_kernel_boundary + T_metadata + T_scan` is a huge chunk of the bill.

So the ASM rewrite was never going to win just because the source file ended in `.s`.

It had to earn every microsecond the same way the C version did.

## What The ASM Version Actually Does

This port is intentionally current-machine oriented rather than trying to be a giant universal portability project.

Right now it does real live checks for the environment this machine actually cares about:

- distro from `/etc/os-release`
- kernel from `uname`
- uptime from `/proc/uptime`
- memory from `/proc/meminfo`
- WM from env
- terminal from env and parent-process inspection
- shell from env
- CPU from CPUID plus sysfs
- GPU from NVIDIA procfs fast path
- packages from pacman, flatpak, and nix paths

That was the right scope for this rewrite.

Trying to rebuild every distro path and every edge-case backend in assembly before getting a fast correct binary would have been a good way to waste time and learn nothing.

## What Was Annoying In Assembly

The obvious pain is readability, but the more interesting pain was correctness under register pressure.

The repeated failures during bring-up were not exotic. They were brutally ordinary:

- caller-saved registers getting clobbered across helper calls
- pointers parked in registers that an instruction like `cpuid` overwrites
- one broken append helper poisoning the whole output path
- path-building bugs that C would make almost boring to fix
- "this should obviously work" logic falling apart because one register got reused three calls earlier

That is the part assembly forces on you. Every assumption has to be earned.

In C you still need to think, but the compiler saves you from a lot of raw bookkeeping mistakes.

In assembly, the bookkeeping is the job.

## Why The Code Has Math Comments

The comments in [`Bfetchasm/fetch.s`](./Bfetchasm/fetch.s) are there because i wanted to prove i wasn't worng to myself "it is fast" is not enough.

Each detector is better thought of as a small cost equation:

- distro
  - `open + read + parse`
- uptime
  - `open + read + integer divisions`
- memory
  - `open + read + scan + scaling`
- CPU
  - `cpuid + tiny sysfs reads`
- packages
  - `directory walks + manifest scans`

That framing is important because it separates:

- real unavoidable work
- avoidable implementation waste

Without that, assembly turns into cargo culting instruction counts while the actual cost is still sitting in the kernel and filesystem.

## Static Linking Was Part Of The Experiment

The assembly binary is built static on purpose.

That gives a cleaner answer to the startup question because it avoids dynamic loader work and keeps the experiment focused on:

- program startup
- syscall path
- parsing and formatting

instead of "what did the runtime loader just do for me before `_start` even ran?"

So this version is useful not just as a rewrite, but as a measurement tool.

## Performance

On my machine, once the assembly version was brought to correctness, it ended up benchmarking slightly faster than the current C build in one direct comparison:

- `./bfetchasm`: about `886 µs`
- `./fetch`: about `927 µs`

That is not some anime 10x ownage number, but it is real enough for me to beat my meat to.

And more importantly, it is informative.

It says:

- yes, there was still a little room left after the C version
- no, assembly did not annihilate the remaining cost
- most of the lower bound is still the real system inspection work

That is exactly the kind of answer worth getting.

## Build

From the repo root:

```bash
make -C Bfetchasm
```

Run it:

```bash
./Bfetchasm/bfetchasm
./Bfetchasm/bfetchasm --version
./Bfetchasm/bfetchasm --help
```

Benchmark it without shell startup:

```bash
hyperfine --shell=none ./Bfetchasm/bfetchasm ./fetch
```

Check that it is really static:

```bash
file ./Bfetchasm/bfetchasm
ldd ./Bfetchasm/bfetchasm || true
```

## What This Version Is Good For

It is good for three things:

### 1. Testing the real lower bound

If you want to know whether the C version is leaving obvious userspace overhead on the table, this version is the bluntest way to ask.

### 2. Studying the exact plumbing

The assembly source makes the syscall boundaries, register lifetimes, buffer writes, and detector flow painfully explicit.

### 3. Being a record of where the cost actually lives

Once this version is correct and still lands near the C version instead of galaxies away from it, you learn something useful:

the hard part is not that C was secretly too high-level. The hard part is that real system detection has a real cost and i wanted to see every messure of that.

That is a much better conclusion than vague "rewrite it in assembly bro" nonsense.
