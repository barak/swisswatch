# SwissWatch

An analog clock for Linux/GTK4 that renders the iconic Swiss Railway
(SBB/CFF/FFS) clock face designed by Hans Hilfiker in the 1940s.

By default the clock appears as an undecorated, transparent circular
window that floats on the desktop without a title bar.  The window can
be moved by dragging anywhere on the clock face.

## Features

- Five clock faces: `swisswatch`, `botta`, `fancy`, `swissclock`, `oclock`
- Shaped (circular, undecorated) window using compositor transparency
- Keyboard shortcuts for resizing, fullscreen, and toggling all modes
- Railroad mode: second hand sweeps 57.5 s then snaps to 12 o'clock,
  minute hand jumps on the minute signal
- GTK4/Cairo rendering; works on X11 (with compositing) and Wayland

## Building

Build dependencies: GTK4, help2man

```sh
autoreconf -fi
./configure
make
make install
```

## Clock faces

| `--name` | Description |
|----------|-------------|
| `swisswatch` | Authentic SBB/CFF/FFS Swiss Railway clock (default) |
| `botta` | Mario Botta's design for the San Francisco MoMA |
| `fancy` | Stylised outlined hands on black |
| `swissclock` | Swiss clock with arrow hands |
| `oclock` | MIT oclock emulation, minimal two-hand clock |

## Options

| Option | Description |
|--------|-------------|
| `--name=NAME` | Select clock face (see above) |
| `--noshape` | Standard decorated rectangular window |
| `--nocircular` | Elliptical face that stretches to fit the window |
| `--norailroad` | Smooth, continuous second-hand motion |
| `--tick=SECONDS` | Update interval (default: 0.06 s) |
| `--fullscreen` | Start in fullscreen mode |

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| Escape, q, Q | Exit |
| +, = | Grow window by 50 px |
| − | Shrink window by 50 px |
| f, F, F11 | Toggle fullscreen |
| r, R | Toggle railroad mode |
| n, N | Cycle to next clock face |
| s, S | Toggle shaped mode |
| c, C | Toggle circular mode|

## History

Originally written by Simon Leinen (EPFL) in 1992 as an Xt/X11
application, inspired by Der Mouse's *mclock* program.
Ported to GTK4/Cairo by Barak A. Pearlmutter in 2026.
