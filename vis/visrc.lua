require('vis')

								-- config --

-- global
vis.events.subscribe(vis.events.INIT, function()
	vis:command("set theme base16-default-dark")
end)
-- per-window
vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	vis:command("set tabwidth 4")
	vis:command("set numbers true")
	vis:command("set autoindent true")
	vis:command("set showspaces false")
	vis:command("set showtabs false")
	vis:command("set expandtab on")
	vis:command("set shell /usr/bin/env sh")
	vis:map(vis.modes.VISUAL," y", '"+y"')
	vis:map(vis.modes.NORMAL, " e", function()
		vis:command("open .")
		vis:feedkeys("<C-w>k")
		vis:command("wq!")
	end, "")
end)

								-- plugins --

-- modal
-- local modal = require('plugins/vis-modal')

-- vis-autoclose
local autoclose = require('plugins/vis-autoclose')

-- colorizer
local colorizer = require('plugins/vis-colorizer')
colorizer.three = false
colorizer.six   = true

-- complete-filename
local completefilename = require('plugins/complete-filename')

-- vis-lspc
local lsp = require('plugins/vis-lspc')

lsp.highlight_diagnostics = 'line'

lsp.ls_map['c'] = {name = 'clangd', cmd = 'clangd --background-index'}
lsp.ls_map['cpp'] = {name = 'clangd', cmd = 'clangd --background-index'}

lsp.ls_map.clangd = {
	formatting_options = {tabSize = 4, insertSpaces = false}
}
