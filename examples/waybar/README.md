# Waybar Lyrics Module

Display synced lyrics in your Waybar using lyricsd's D-Bus interface.

## Setup

1. Copy the script and make it executable:

```sh
mkdir -p ~/.config/waybar/scripts
cp lyricsd-waybar.sh ~/.config/waybar/scripts/
chmod +x ~/.config/waybar/scripts/lyricsd-waybar.sh
```

2. Add the module to your Waybar config (`~/.config/waybar/config.jsonc`):

```jsonc
"modules-center": ["custom/lyrics"],

"custom/lyrics": {
  "exec": "~/.config/waybar/scripts/lyricsd-waybar.sh",
  "return-type": "json",
  "format": "{}",
  "tooltip": true,
  "max-length": 80,
  "on-click": "playerctl play-pause"
}
```

3. Add the styles to your Waybar CSS (`~/.config/waybar/style.css`) — see `style.css`.

4. Make sure `lyricsd` is running, then reload Waybar.

## How it works

The script uses `gdbus monitor` to listen for `LyricsLineChanged` signals from the `org.lyricsd.Lyrics` D-Bus service. Each signal is parsed and output as a JSON line for Waybar's custom module to consume.

Clicking the module toggles play/pause via `playerctl`.
