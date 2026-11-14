--[[
    viscosity: tree
    TODO: add desc
]]

local base   = (...):match('^(.*)%.[^.]+$')
local config = require(base .. '.config')
local state  = require(base .. '.state')
local fs     = require(base .. '.fs')

local M = {}

-- NOTE(ZackeryRSmith): These strings are to be kept identical to those of config.styles
M.KIND = {
    file              = 'file',
    directory         = 'directory',
    special_directory = 'special_directory',
    symlink           = 'symlink',
}

---@class TreeEntry : FsEntry
---@field depth integer
---@field expanded? boolean

--- paths ----------------------------------------------------------------------

function M.absolute(path)
    if path:sub(1, 1) ~= '/' then
        local cwd = os.getenv('PWD') or '.'
        path = (path == '.') and cwd or (cwd .. '/' .. path)
    end
    if #path > 1 then path = path:gsub('/+$', '') end
    if path == '' then path = '/' end
    return path
end

function M.is_hidden(entry)
    return entry.kind ~= M.KIND.special_directory and entry.name:sub(1, 1) == '.'
end

function M.index_of_path(path)
    for i, entry in ipairs(state.entries) do
        if entry.path == path then return i end
    end
    return nil
end

function M.parent_index(index)
    local entry = state.entries[index]
    if not entry then return nil end
    for i = index - 1, 1, -1 do
        if state.entries[i].depth < entry.depth then return i end
    end
    return nil
end

--- expand / collapse ----------------------------------------------------------

---@param path string
---@param depth integer The depth the entires are at rel. to the root.
---@return TreeEntry[]
local function entries_from_dir(path, depth)
    local children = {}
    for _, e in ipairs(fs.list(path)) do
        local entry = {
            name       = e.name,
            path       = e.path,
            kind       = e.kind,
            executable = e.executable,
            depth      = depth,
            expanded   = false,
        }
        if config.show_hidden or not M.is_hidden(entry) then
            children[#children + 1] = entry
        end
    end
    return children
end

---@return boolean has changed?
function M.expand(index)
    local entry = state.entries[index]
    if not entry or entry.kind ~= M.KIND.directory or entry.expanded then
        return false
    end
    -- insert at a moving index, otherwise the children come out reversed
    for i, child in ipairs(entries_from_dir(entry.path, entry.depth + 1)) do
        table.insert(state.entries, index + i, child)
    end
    entry.expanded = true
    return true
end

---@return boolean has changed?
function M.collapse(index)
    local entry = state.entries[index]
    if not entry or not entry.expanded then return false end

    local last = index
    while state.entries[last + 1] and state.entries[last + 1].depth > entry.depth do
        last = last + 1
    end
    -- remove back to front so indices don't shift under us
    for i = last, index + 1, -1 do
        table.remove(state.entries, i)
    end
    entry.expanded = false
    return true
end

-- NOTE(iambird): Children are inserted right after their parent
local function expand_where(predicate)
    local i = 1
    while i <= #state.entries do
        local entry = state.entries[i]
        if entry.kind == M.KIND.directory and not entry.expanded and predicate(entry) then
            M.expand(i)
        end
        i = i + 1
    end
end

-- build --------------------------------------------------------------

---@param path string
---@param expanded? table<string, boolean> paths to re-expand
function M.build(path, expanded)
    state.root = path
    state.entries = {}

    local parent = fs.dirname(path)
    if parent ~= path then
        state.entries[1] = {
            name     = '../',
            path     = parent,
            kind     = M.KIND.special_directory,
            depth    = 0,
            expanded = false,
        }
    end
    for _, child in ipairs(entries_from_dir(path, 0)) do
        state.entries[#state.entries + 1] = child
    end

    if expanded then
        expand_where(function(entry) return expanded[entry.path] end)
    elseif config.auto_expand_depth then
        expand_where(function(entry)
            return entry.depth < config.auto_expand_depth
                and not M.is_hidden(entry)
                and #state.entries < config.auto_expand_max
        end)
    end
end

function M.refresh()
    if not state.root then return end
    local expanded = {}
    for _, entry in ipairs(state.entries) do
        if entry.expanded then expanded[entry.path] = true end
    end
    M.build(state.root, expanded)
end

---@return boolean has changed?
function M.up()
    if not state.root then return false end
    local parent = fs.dirname(state.root)
    if parent == state.root then return false end
    M.build(parent)
    return true
end

return M
