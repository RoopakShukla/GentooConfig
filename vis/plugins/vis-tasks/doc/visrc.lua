require'vis'

plugins = {
        tasks = require'tasks',
}

vis.events.subscribe(vis.events.INIT, function()
        plugins.tasks.setup()
end)
