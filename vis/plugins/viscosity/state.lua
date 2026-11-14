--[[
    viscosity: state
    TODO: add desc
]]

---@class TreeState
---@field win any?                   the tree window, nil when closed
---@field prev_win any?              window that was focused when the tree opened
---@field file any?                  the tree's fake file
---@field root string?               absolute path the tree is rooted at
---@field pending boolean            true when waiting to claim a new window
---@field entries TreeEntry[]        entries[n] is what is shown on buffer line n
---@field clear fun(self: TreeState) clears the current tree state

---@class M : TreeState
local M = {
    win      = nil,
    file     = nil,
    prev_win = nil,
    root     = nil,
    pending  = false,
    entries  = {},
}

function M:clear()
    self.win      = nil
    self.file     = nil
    self.prev_win = nil
    self.root     = nil
    self.pending  = false
    self.entries  = {}
end

return M
