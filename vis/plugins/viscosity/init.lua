--[[
    viscosity
    an oil-style file tree for the vis editor
]]
-- TODO: (in no real order)
--   [ ] HELP window inside tree (open with ?)
--   [ ] allow opening the tree on the directory of the current file
--   [ ] edit the buffer to change the file system (create, rename, move, delete)
--   [ ] confirm pending changes before applying them
--   [ ] make window width configurable (upstream issue martanne/vis#1426)
--   [ ] show symlink targets
--   [ ] combine .c/.h on one line with a selector
--   [ ] sort options (directories first, by name, by modified time)
--   [ ] nerdfont/icon support


-- HACK: Inform LuaLS that `vis` is injected at runtime
---@type any
_G.vis = _G.vis

local base   = (...)
local config = require(base .. '.config')
local state  = require(base .. '.state')
local tree   = require(base .. '.tree')
local view   = require(base .. '.view')
local fs     = require(base .. '.fs')

local M = config

-- if the path is a directory it is returned
-- if the path is a file it's containing directory is returned
local function nearest_dir(path)
    local spath = (path or '.'):gsub('%s+$', ''):gsub('[\r\n]', '')
    if spath == '' then spath = '.' end

    if fs.is_dir(spath) then return spath end
    local dir = fs.dirname(spath)
    if fs.is_dir(dir) then return dir end
    return nil
end

function M.open(path)
    local dir = nearest_dir(path)
    if not dir then
        vis:info('viscosity: not a directory: ' .. tostring(path))
        return
    end
    dir = tree.absolute(dir)

    -- a tree is already open so we'll just re-root it
    if view.window_alive(state.win) then
        view.update(function() tree.build(dir) end)
        return
    end

    tree.build(dir)
    state.prev_win = vis.win
    state.pending = true
    if not vis:command('vnew') then state.pending = false end
end

--- events ---------------------------------------------------------------------

vis.events.subscribe(vis.events.INIT, function()
    if config.command then
        vis:command_register(config.command, function(argv)
            M.open(argv[1])
            return true
        end, 'Open the file tree')
    end
end)

vis.events.subscribe(vis.events.START, function()
    -- see: config.lua
    for key, default in pairs(config.default_styles()) do
        local style = config.styles[key]
        if style == nil then style = default end
        if type(style) == 'string' then
            style = vis.ui:style_push(style)
        end
        config.styles[key] = style
    end

    if not config.open_on_empty then return end

    local win = vis.win
    local file = win and win.file
    if file and not file.path and file.size == 0 then
        M.open('.')
        -- the blank window is redundant now
        if win ~= state.win and view.window_alive(win) then
            win:close(false)
        end
    end
end)

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
    if not state.pending or not win then return end
    state.pending = false
    view.attach(win)
end)

vis.events.subscribe(vis.events.WIN_CLOSE, function(win)
    if state.win and win == state.win then
        state:clear()
    end
end)

vis.events.subscribe(vis.events.FILE_SAVE_PRE, function(file)
    if (state.file and file == state.file) or file.name == config.buffer_name then
        vis:info(config.buffer_name .. ' cannot be written')
        return false
    end
end)

vis.events.subscribe(vis.events.WIN_HIGHLIGHT, view.highlight)

return M
