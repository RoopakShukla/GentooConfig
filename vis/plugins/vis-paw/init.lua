local paw = {}

-- match the modes to their names
local modes = {
  [vis.modes.NORMAL] = 'NORMAL',
  [vis.modes.OPERATOR_PENDING] = 'N',
  [vis.modes.VISUAL] = 'VISUAL',
  [vis.modes.VISUAL_LINE] = 'VL',
  [vis.modes.INSERT] = 'INSERT',
  [vis.modes.REPLACE] = 'REPLACE'
}

-- default configuration options
paw.config = {
  style_normal = 'fore:black,back:blue',
  style_visual = 'fore:black,back:yellow',
  style_insert = 'fore:black,back:green',
  style_replace = 'fore:black,back:magenta',

  separator_left = ' ',
  separator_right = ' « ',

  modules_left = {
    'mode',
    'file'
  },
  modules_right = {
    'flags',
    'syntax',
    'percent',
    'progress'
  }
}

-- override config
function paw.setup(opt)
  if opt then
    for k, v in pairs(opt) do
      paw.config[k] = v
    end
  end
end

-- module definition functions
local modules = {}

modules.mode = function(c)
  local mode = modes[vis.mode]
  if mode ~= '' and vis.win == c.win then
    c.modelength = #mode + 1
    table.insert(c.parts, ' ' .. mode .. ' ')
  end
end

modules.file = function(c)
  local win = c.win
  table.insert(c.parts, (win.file.name or '[SCRATCH]'):gsub("^/home/%w+/", "~/") ..
    (win.file.modified and ' +' or '') .. (vis.recording and ' @' or ''))
end

modules.syntax = function(c)
  local syn = c.win.syntax
  table.insert(c.parts, syn)
end

modules.flags = function(c)
  local keys = vis.input_queue
  local count = vis.count
  if keys ~= '' then
    table.insert(c.parts, keys)
  elseif count then
    table.insert(c.parts, count)
  end
end

modules.percent = function(c)
  local file = c.win.file
  local selection = c.win.selection
  local size = file.size
  local pos = selection.pos or 0
  table.insert(c.parts, (size == 0 and "0" or math.ceil(pos/size*100)) .. "%")
end

modules.progress = function(c)
  local selection = c.win.selection
  table.insert(c.parts, selection.line .. ', ' .. selection.col)
end

-- check and build the modules
local function build(win, list)
  local parts = {}
  local c = { win = win, parts = parts }

  for _, m in ipairs(list) do
    local module = modules[m]
    if module then module(c) else
      vis:message("unknown module: " .. m)
    end
  end
  return parts, c.modelength
end

-- assemble the statusbar
vis.events.subscribe(vis.events.WIN_STATUS, function(win)
  local left_parts, modelen = build(win, paw.config.modules_left)
  local right_parts = build(win, paw.config.modules_right)
  local left = table.concat(left_parts, paw.config.separator_left) .. ' '
  local right = ' ' .. table.concat(right_parts, paw.config.separator_right) .. ' '
  win:status(left, right)

  -- thank you @dther!: https://github.com/martanne/vis/pull/1180
  win.STYLE_MODE_NORMAL = win.STYLE_LEXER_MAX - 1
  win.STYLE_MODE_VISUAL = win.STYLE_LEXER_MAX - 2
  win.STYLE_MODE_INSERT = win.STYLE_LEXER_MAX - 3
  win.STYLE_MODE_REPLACE = win.STYLE_LEXER_MAX - 4
  win:style_define(win.STYLE_MODE_NORMAL, paw.config.style_normal)
  win:style_define(win.STYLE_MODE_VISUAL, paw.config.style_visual)
  win:style_define(win.STYLE_MODE_INSERT, paw.config.style_insert)
  win:style_define(win.STYLE_MODE_REPLACE, paw.config.style_replace)

  local modestyle = {
    [vis.modes.NORMAL] = win.STYLE_MODE_NORMAL,
    [vis.modes.OPERATOR_PENDING] = win.STYLE_MODE_NORMAL,
    [vis.modes.VISUAL] = win.STYLE_MODE_VISUAL,
    [vis.modes.VISUAL_LINE] = win.STYLE_MODE_VISUAL,
    [vis.modes.INSERT] = win.STYLE_MODE_INSERT,
    [vis.modes.REPLACE] = win.STYLE_MODE_REPLACE
  }

  local style = modestyle[vis.mode]
  if style and vis.win == win and modelen then
    for c = 0, modelen do
      win:style_pos(style, c, win.height - 1)
    end
  end
end)

return paw
