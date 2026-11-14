--[[
    viscosity: config
    default settings, change them from your visrc.lua:

        local viscosity = require('plugins/viscosity')
        viscosity.show_hidden = true
]]

require('vis')

---@class ViscosityConfig
local M = {
    --- Open a tree when vis starts with an empty, unnamed buffer.
    open_on_empty     = true,
    --- Close the tree after a file is opened from it.
    close_on_open     = true,
    --- How deep directories are expanded when the tree opens.
    --- `0` expands nothing, `1` expands the directly under the root,
    --- `math.huge` expands everything.
    auto_expand_depth = 0,
    --- Stop auto expanding once the tree has this many entries.
    auto_expand_max   = 500,
    --- Show files and directories whose name starts with a period.
    show_hidden       = true,
    --- Name given to the tree's buffer
    buffer_name       = "viscosity",
    --- Character(s) to use when indenting
    indent            = "\t",
    --- The command which gets registered to open Viscosity.
    command           = "tree",

    keys = {
        interact      = '<Enter>',
        parent        =  '-',
        reroot        = 'cr',
        refresh       = 'R',
        toggle_hidden = 'g.',
        close         = 'q'
    },

    preview = {
        enabled = false,
        max_bytes = 65536,
    },

    window_options = {
        cursorline = true,
        showeof = false,
        numbers = true,
        relativenumbers = false,
        showtabs = false,
        showspaces = false,
        shownewlines = false,
        colorcolumn = 0,
    },

    -- NOTE(ZackeryRSmith): Style slots for each entry kind. These start empty
    -- because lexer styles aren't populated yet. The start handler (init.lua)
    -- fills in whatever is still unset, using M.default_styles()
    styles = {
        file = nil,
        directory = nil,
        special_directory = nil,
        symlink = nil,
        hidden = nil,
    },
}

function M.default_styles()
    local lexers = vis.lexers
    return {
        directory = "fore:blue",
        special_directory = lexers.STYLE_COMMENT or "fore:#9a9c8b",
        symlink = lexers.STYLE_STRING or "fore:cyan",
        hidden = lexers.STYLE_COMMENT or "fore:#9a9c8b",
    }
end

return M
