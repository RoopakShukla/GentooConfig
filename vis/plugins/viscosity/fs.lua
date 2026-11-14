--[[
    viscosity: fs
    file system helpers for vis-filetree
]]

local M = {}

-- Used when parsing the output of `ls -F`
local MARKERS = {
    ['/'] = { kind = 'directory' },               -- directory
    ['@'] = { kind = 'symlink' },                 -- symlink
    ['*'] = { kind = 'file', executable = true }, -- executable
    ['|'] = { kind = 'file' },                    -- fifo
    ['='] = { kind = 'file' },                    -- socket
}

---@param s string
---@return string
local function shell_quote(s)
    return "'" .. s:gsub("'", "'\\''") .. "'"
end

---@param path string
---@return boolean
function M.is_dir(path)
    local res = os.execute('test -d ' .. shell_quote(path))
    return res == true or res == 0
end

function M.join(dir, name)
    if dir:sub(-1) == '/' then return dir .. name end
    return dir .. '/' .. name
end


---@class FsEntry
---@field name string
---@field path string
---@field kind 'directory'|'special-directory'|'file'|'symlink'|
---@field executable? boolean
---@field target? string
---@field broken? boolean
---@field size? integer

---@param path string
---@return FsEntry[]
function M.list(path)
    local entries = {}
    local p = io.popen('ls -1AF -- ' .. shell_quote(path) .. ' 2>/dev/null')
    if not p then return entries end

    for line in p:lines() do
        local info = MARKERS[line:sub(-1)]
        local name = info and line:sub(1, -2) or line

        if name ~= '' then
            entries[#entries + 1] = {
                name = name,
                path = M.join(path, name),
                kind = info and info.kind or 'file',
                executable = info and info.executable
            }
        end
    end

    return entries
end

---Return the directory part of a path, like dirname(1).
---@param path string
---@return string
function M.dirname(path)
    local dir = path:match('^(.*)/[^/]*$')
    if not dir then return '.' end
    if dir == '' then return '/' end
    return dir
end

return M
