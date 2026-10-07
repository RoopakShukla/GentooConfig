require'vis'

local M = {}
local function system(cmd)
	local f = assert(io.popen(cmd, 'r'))
	local s = assert(f:read'*a')
	f:close()
	return s:sub(1, #s-1)
end

local function add_date(x, at, align)
	local date = at .. system('date +%F')
	return x .. string.rep(' ', align - #x - #date + 2) .. date
end

function M.insert()
	local win = vis.win
	local file = win.file
	local task = ' - ' .. M.sym .. ' '
	for sel in win:selections_iterator() do
		local pos = sel.pos
		local line = file.lines[sel.line]
		local _, _, p = line:find'^(%s*)'
		local _, _, q = line:find'%s*(.*)$'
		file.lines[sel.line] = p .. task .. q
		sel.pos = pos + #p + #task - 1
	end
end

function M.toggle(x, m)
	local win = vis.win
	local file = win.file
	for sel in win:selections_iterator() do
		local line = file.lines[sel.line]
		local long  = '(.+' ..   x   .. '.-)%s+@.+$'
		local short = '(.+' .. M.sym .. '.+)%s*$'
		local s, e, p, r

		s, e, p = line:find(long)
		if p then
			r = p:gsub(x, M.sym)
			goto replace
		end

		s, e, p = line:find(short)
		if p then
			r = add_date(p:gsub(M.sym, x), ' @' .. m .. ' ', 80)
			goto replace
		end
		goto done

		::replace::
		local pos = sel.pos
		file.lines[sel.line] = r
		sel.pos = s + pos - 1
	end
	::done::
end

function M.setup(t, x)
	local map = t or {
		{' tb', '⚠', 'blocked' , 'task: toggle blocked'},
		{' tc', '✘', 'canceled', 'task: toggle canceled'},
		{' td', '✔', 'done'    , 'task: toggle done'},
	}
        M.sym = x or '□'
	vis:map(vis.modes.NORMAL, ' tn', M.insert, 'task: create new task')
	for k, v in pairs(map) do
		assert(#v == 4, 'task: expected <key> <sym> <tag> <help>')
		vis:map(vis.modes.NORMAL, v[1], function()
			M.toggle(v[2], v[3])
		end, v[4])
	end
end

return M
