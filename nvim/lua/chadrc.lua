-- This file needs to have same structure as nvconfig.lua 
-- https://github.com/NvChad/ui/blob/v3.0/lua/nvconfig.lua
-- Please read that file to know all available options :( 

---@type ChadrcConfig
local M = {}

M.base46 = {
	theme = "catppuccin",
  -- transparency = true,

	hl_override = {
		Comment = { italic = true },
		["@comment"] = { italic = true },
	},
}

M.nvdash = { load_on_startup = true }
M.ui = {
      tabufline = {
         lazyload = true,
         treeOffsetFt = "NvimTree",
         order = {"treeOffset","buffers","tabs"},
     },
}

M.colorify = {
    enabled = true,
    mode = "bg",
}

M.lsp = { signature = true}

return M
