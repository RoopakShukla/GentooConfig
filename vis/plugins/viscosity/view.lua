--[[
    viscosity: view
    renders TreeEntry's into a buffer and handles input
]]

require('vis')
local base   = (...):match('^(.*)%.[^.]+$')
local config = require(base .. '.config')
local state  = require(base .. '.state')
local tree   = require(base .. '.tree')
local fs     = require(base .. '.fs')

local M = {}
local KIND = tree.KIND

--- rendering ------------------------------------------------------------------

local function entry_text(entry)
    local suffix = entry.kind == KIND.directory and '/' or ''
    return config.indent:rep(entry.depth) .. entry.name .. suffix
end

-- first real entry, skipping the '../' line
local function default_line()
    local first = state.entries[1]
    if first and first.kind == KIND.special_directory and #state.entries > 1 then
        return 2
    end
    return 1
end

local function render(win)
    local file = win.file
    local lines = {}
    for i, entry in ipairs(state.entries) do
        lines[i] = entry_text(entry)
    end
    file:delete(0, file.size)
    file:insert(0, table.concat(lines, '\n'))
    file.modified = false
end

--- Edit the tree and redraw the window.
---@param mutate fun(): boolean? edits state.entries; return false to skip the redraw
---@param focus? string path to put the cursor on
function M.update(mutate, focus)
    local win = state.win
    if not win then return end

    local current = state.entries[win.selection.line]
    local keep = current and current.path

    if mutate() == false then return end
    render(win)

    local index = (focus and tree.index_of_path(focus))
        or (keep and tree.index_of_path(keep))
        or default_line()
    win.selection:to(index, 1)
end

--- key maps -------------------------------------------------------------------

local function guarded(fn)
    return function()
        local ok, err = pcall(fn)
        if not ok then vis:info('viscosity: ' .. tostring(err)) end
    end
end

local function cursor_index()
    return state.win.selection.line
end

--- @return TreeEntry|nil, integer|nil (entry, index)
local function current_entry()
    local win = state.win
    if not win then return nil end

    local index = cursor_index()
    return state.entries[index], index
end

local actions = {}

function actions.interact()
    local entry, index = current_entry()
    if not entry then return end

    M.update(function()
        if entry.kind == KIND.file then return false end

        if entry.kind == KIND.special_directory then
            tree.build(entry.path)
        elseif entry.kind == KIND.directory then
            if entry.expanded then tree.collapse(index) else tree.expand(index) end
        elseif entry.kind == KIND.symlink and fs.is_dir(entry.path) then
            tree.build(entry.path)
        end

        return true
    end, state.root)
    if entry.kind == KIND.file then M.open_file(entry.path) end
end

function actions.parent()
    M.update(function() return tree.up() end, state.root)
end

function actions.refresh()
    M.update(function() tree.refresh() end)
end

function actions.reroot()
    local entry, _ = current_entry()
    if not entry then return end

    state.root = fs.dirname(entry.path)
    M.update(function() tree.refresh() end)
end

function actions.toggle_hidden()
    config.show_hidden = not config.show_hidden
    M.update(function() tree.refresh() end)
end

function actions.close()
    M.close()
end

local HELP = {
    interact      = 'Interact with directory/file/symlink',
    parent        = 'Go up one directory',
    refresh       = 'Refresh the tree',
    reroot        = 'Set current directory as the root',
    toggle_hidden = 'Toggle hidden files',
    close         = 'Close the tree',
}

local function bind_keys(win)
    for name, fn in pairs(actions) do
        local key = config.keys[name]
        if key then
            win:map(vis.modes.NORMAL, key, guarded(fn), HELP[name])
        end
    end
end

--- windows --------------------------------------------------------------------

--- Turn a window into the tree.
function M.attach(win)
    state.win = win
    win.file = config.buffer_name
    state.file = win.file
    win.width = 50

    for name, value in pairs(config.window_options) do
        local ok = pcall(function() win.options[name] = value end)
        if not ok then vis:info('viscosity: bad window option ' .. name) end
    end

    bind_keys(win)
    render(win)
    win.selection:to(default_line(), 1)
end

function M.window_alive(win)
    if not win then return false end
    for w in vis:windows() do
        if w == win then return true end
    end
    return false
end

function M.close()
    local win = state.win
    if not win then return end
    -- the WIN_CLOSE handler also clears the state
    if win:close(true) then
        state:clear()
    else
        vis:info('viscosity: cannot close the last window')
    end
end

function M.open_file(path)
    local prev = state.prev_win
    if M.window_alive(prev) then
        prev.file = path
    else
        vis:command('open ' .. path)
    end
    if config.close_on_open then M.close() end
end

--- highlighting ---------------------------------------------------------------

local function style_id(entry)
    if tree.is_hidden(entry) and config.styles.hidden then
        return config.styles.hidden
    end
    return config.styles[entry.kind]
end

-- NOTE(iambird): This is attached to a WIN_HIGHLIGHT event.
function M.highlight(win)
    if win ~= state.win then return end
    local file = win.file
    if not file then return end

    -- styles are cleared on every redraw, so only style what is visible
    local first, last = 1, #state.entries
    local viewport = win.viewport and win.viewport.lines
    if viewport and viewport.start and viewport.finish then
        first = math.max(first, viewport.start)
        last = math.min(last, viewport.finish + 1)
    end

    for lineno = first, last do
        local entry = state.entries[lineno]
        local id = entry and style_id(entry)
        if id then
            local start = file:offset_from_line_column(lineno)
            if start then
                win:style(id, start, start + #entry_text(entry))
            end
        end
    end
end

return M
