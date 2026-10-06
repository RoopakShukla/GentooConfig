require('vis')

								-- config --

-- global
vis.events.subscribe(vis.events.INIT, function()
	vis:command("set theme base16-default-dark")
end)
-- per-window
vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	vis:command("set tabwidth 4")
	vis:command("set relativenumbers on")
	vis:command("set autoindent on")
	vis:command("set showspaces off")
	vis:command("set showtabs off")
	vis:command("set expandtab off")
	vis:command("set shell sh")
	vis:map(vis.modes.VISUAL," y", '"+y"')
	vis:map(vis.modes.NORMAL, " ff", function()
		vis:command("open .")
		vis:feedkeys("<C-w>k")
		vis:command("wq!")
	end, "")
end)

								-- plugins --

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

-- Optional: Highlight the line number when there is a warning/error
lsp.highlight_diagnostics = 'line'

-- Map C and C++ lexers to clangd
lsp.ls_map['c'] = {name = 'clangd', cmd = 'clangd --background-index'}
lsp.ls_map['cpp'] = {name = 'clangd', cmd = 'clangd --background-index'}

lsp.ls_map.clangd = {
	formatting_options = {tabSize = 4, insertSpaces = false}
}

lsp.ls_map.lua = {
	name = 'lua-language-server',
	cmd = 'lua-language-server',
	settings = {
		Lua = {diagnostics = { globals = {'vis'}}, telemetry = {enable = false}},
	},
	formatting_options = {tabSize = 4, insertSpaces = false},
}
