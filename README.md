# lyricsd

A daemon that fetches synced lyrics for the current track playing in any MPRIS-compatible media player. It exposes the lyrics via D-Bus so other programs can display them.

## How it works

1. Connects to any MPRIS-compatible player (Spotify, VLC, mpv, etc.)
2. Fetches synced lyrics from LRCLIB
3. Synchronizes lyrics with playback position
4. Exposes the current lyric line via D-Bus

## Requirements

- sdbus-c++ (D-Bus C++ library)
- libcurl
- CMake 3.16+
- C++17 compiler

Install on Arch Linux:

```
sudo pacman -S sdbus-c++ curl cmake
```

Install on Ubuntu/Debian:

```
sudo apt install libsdbus-c++-dev libcurl4-openssl-dev cmake
```

## Build

```
cmake -B build
cmake --build build
```

The binary will be at `build/lyricsd`.

## Run

```
./build/lyricsd
```

Make sure a media player is running before starting lyricsd.

## D-Bus API

lyricsd exposes a D-Bus service that other programs can use.

### Service Details

| Property | Value |
|----------|-------|
| Service Name | `org.lyricsd.Lyrics` |
| Object Path | `/org/lyricsd/Lyrics` |
| Interface | `org.lyricsd.Lyrics` |

### Properties

#### CurrentLine (read-only, string)

The current lyric line being displayed. Empty string when paused or no player is active.

You can read it directly:

```
gdbus call --session \
  --dest org.lyricsd.Lyrics \
  --object-path /org/lyricsd/Lyrics \
  --method org.freedesktop.DBus.Properties.Get \
  org.lyricsd.Lyrics CurrentLine
```

### Signals

#### LyricsLineChanged(string line)

Emitted when the current lyric line changes. The argument is the new lyric text. Empty string means playback is paused or player was closed.

Example using `dbus-monitor`:

```
dbus-monitor --session "type='signal',sender='org.lyricsd.Lyrics'"
```

### Integration Examples

See the `examples/waybar/` directory for a Waybar module that displays lyrics.

## MPRIS Callbacks

lyricsd listens to these MPRIS events from the player:

- **Track changed**: Fetches new lyrics and starts sync
- **Seeked**: Adjusts lyrics position to match
- **Playback status**: Pauses/resumes lyrics sync
- **Player closed**: Clears current lyrics

## License

MIT
