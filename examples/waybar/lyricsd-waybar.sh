SERVICE="org.lyricsd.Lyrics"
OBJECT="/org/lyricsd/Lyrics"
INTERFACE="org.lyricsd.Lyrics"
STATE_FILE="/tmp/lyricsd_current"

# I have a custom script for nepalidate date 
NEPALIDATE_SCRIPT="$HOME/.config/waybar/scripts/nepalidate/nepalidate.js"

cleanup() {
  [[ -n $DBUS_PID ]] && kill "$DBUS_PID" 2>/dev/null
  rm -f "$STATE_FILE"
}
trap cleanup EXIT

echo "" > "$STATE_FILE"

start_dbus_monitor() {
  while ! gdbus introspect --session --dest "$SERVICE" --object-path "$OBJECT" &>/dev/null; do
    echo '{"text": "♫ waiting...", "class": "waiting"}'
    sleep 2
  done

  dbus-monitor --session \
    "type='signal',sender='$SERVICE',interface='$INTERFACE',member='LyricsLineChanged',path='$OBJECT'" 2>/dev/null | \
    while IFS= read -r line; do
      if [[ "$line" =~ ^[[:space:]]+string\ \"(.*)\"$ ]]; then
        lyric="${BASH_REMATCH[1]}"
        echo "$lyric" > "$STATE_FILE"
      fi
    done
}

start_dbus_monitor &
DBUS_PID=$!

get_nepalidate() {
  if [[ -x "$NEPALIDATE_SCRIPT" ]]; then
    node "$NEPALIDATE_SCRIPT" 2>/dev/null
  else
    date +"[%H:%M]"
  fi
}

escape_json() {
  echo "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

while true; do
  current_lyrics=$(cat "$STATE_FILE" 2>/dev/null)

  if [[ -z "$current_lyrics" ]]; then
    nepali_date=$(get_nepalidate)
    nepali_escaped=$(escape_json "$nepali_date")
    echo "{\"text\": \"${nepali_escaped}\", \"tooltip\": \"${nepali_escaped}\", \"class\": \"idle\"}"
  else
    lyrics_escaped=$(escape_json "$current_lyrics")
    echo "{\"text\": \"♫ ${lyrics_escaped}\", \"tooltip\": \"${lyrics_escaped}\", \"class\": \"playing\"}"
  fi

  sleep 1
done
