require('vis')

--        general conf         --
require('plugins/vis-commentary')()
local panel = require('plugins/vis-paw')
local color = require('plugins/colorizer')
local tasks = require('plugins/vis-tasks')
local autoclose = require('plugins/vis-autoclose')
local completefilename = require('plugins/complete-filename')
local lsp = require('plugins/vis-lspc')

vis.events.subscribe(vis.events.INIT, function()
	vis:command('set theme base16-default-dark')
	tasks.setup()
end)

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	vis:command('set relativenumbers on')
	vis:command('set tabwidth 4')
	vis:command('set expandtab off')
	vis:command('set showtabs false')
	vis:command('set autoindent on')
	vis:map(vis.modes.VISUAL," y", '"+y"')
end)

--        plugins conf        --
panel.config = {
  style_normal = 'fore:black,back:blue',
  style_visual = 'fore:black,back:yellow',
  style_insert = 'fore:black,back:green',
  style_replace = 'fore:black,back:magenta',

  separator_left = ' ',
  separator_right = ' | ',

  modules_left = {
    'mode',
    'file'
  },
  modules_right = {
    'flags',
    'syntax',
    'progress',
    'percent'
  }
}

lsp.highlight_diagnostics = 'line'

-- Map C and C++ lexers to clangd
lsp.ls_map['c'] = {name = 'clangd', cmd = 'clangd --background-index'}
lsp.ls_map['cpp'] = {name = 'clangd', cmd = 'clangd --background-index'}

lsp.ls_map.clangd = {
	formatting_options = {tabSize = 4, insertSpaces = false}
}
