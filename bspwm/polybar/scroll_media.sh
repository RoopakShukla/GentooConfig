#!/bin/sh

WIDTH=40
PAD=" • "
offset=0
last_title=""

while true; do
    status=$(playerctl status 2>/dev/null)

    # Exit when nothing is playing/paused so Polybar hides the module
    case "$status" in
        Playing|Paused) ;;
        *) exit 0 ;;
    esac

    title=$(playerctl metadata --format '{{ title }}' 2>/dev/null)
    [ -z "$title" ] && sleep 1 && continue

    # Reset scroll position when song changes
    if [ "$title" != "$last_title" ]; then
        offset=0
        last_title="$title"
    fi

    len=${#title}

    if [ "$len" -le "$WIDTH" ]; then
        out="$title"
    else
        full="${title}${PAD}${title}"
        # Extract substring using POSIX cut (1-indexed)
        start=$((offset + 1))
        end=$((offset + WIDTH))
        out=$(printf '%s' "$full" | cut -c"${start}-${end}")
        
        if [ "$status" = "Playing" ]; then
            offset=$(( (offset + 1) % (len + ${#PAD}) ))
        fi
    fi

    if [ "$status" = "Paused" ]; then
        printf '%%{F#6c7086}   %s%%{F-}\n' "$out"
    else
        printf '   %s\n' "$out"
    fi

    sleep 0.35
done
