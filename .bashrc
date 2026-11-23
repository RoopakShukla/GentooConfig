# .bashrc

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

alias ls='ls --color=always'
alias xi='sudo xbps-install'

export EDITOR="vis"
export VISUAL="vis"

n() {
    if [ "${NNNLVL:-0}" -ge 1 ]; then
        echo "nnn is already running"
        return
     Blow
    fi

    export NNN_TMPFILE="${XDG_CONFIG_HOME:-$HOME/.config}/nnn/.lastd"

    nnn -He "$@"

    if [ -f "$NNN_TMPFILE" ]; then
        . "$NNN_TMPFILE"
        rm -f "$NNN_TMPFILE"
    fi
}

export COLORTERM=truecolor

PS1='[\W] [η] '
