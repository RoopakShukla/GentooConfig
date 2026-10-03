require "nvchad.mappings"

-- add yours here
local map = vim.keymap.set

map("n", ";", ":", { desc = "CMD enter command mode" })
map("i", "jk", "<ESC>")

-- map({ "n", "i", "v" }, "<C-s>", "<cmd> w <cr>")

-- Python
vim.api.nvim_create_autocmd("FileType", {
    pattern = "python", -- Python files ONLY
    callback = function()
        vim.keymap.set("n", "<leader>rp", function()
            vim.cmd("silent w")

            local file = vim.fn.expand("%")
            local command = string.format("python3 -u %s", file)

            vim.cmd("split")
            vim.cmd("term " .. command)

            vim.cmd("startinsert")
        end, { desc = "Run Python file in terminal split", buffer = true })
    end,
})

-- Run C files
vim.api.nvim_create_autocmd("FileType", {
    pattern = "c",
    callback = function()
        vim.keymap.set("n", "<leader>rc", function()
            vim.cmd("silent w")

            local file = vim.fn.expand("%")
            local output_binary = vim.fn.expand("%:r")

            -- Compile && Run && Remove binary
            local command =
                string.format("gcc %s -o %s && ./%s; rm %s", file, output_binary, output_binary, output_binary)

            vim.cmd("split")
            vim.cmd("term " .. command)

            vim.cmd("startinsert")
        end, { desc = "Compile, Run, and Clean C file", buffer = true })
    end,
})
