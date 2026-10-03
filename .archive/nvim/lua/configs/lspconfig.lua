local nvconfig = require "nvchad.configs.lspconfig"

-- Servers using default configuration
local default_servers = { "html", "cssls", "clangd", "lua_ls" }

for _, lsp in ipairs(default_servers) do
  vim.lsp.config[lsp] = {
    on_attach = nvconfig.on_attach,
    on_init = nvconfig.on_init,
    capabilities = nvconfig.capabilities,
  }
  vim.lsp.enable(lsp)
end

-- Configure pylsp (Autocompletion, Hover, Definitions)
vim.lsp.config.pylsp = {
  on_attach = nvconfig.on_attach,
  on_init = nvconfig.on_init,
  capabilities = nvconfig.capabilities,
  settings = {
    pylsp = {
      plugins = {
        -- Disable linters/formatters in pylsp to prevent duplicate diagnostics with Ruff
        pycodestyle = { enabled = false },
        flake8 = { enabled = false },
        mccabe = { enabled = false },
        yapf = { enabled = false },
        autopep8 = { enabled = false },
        rope_completion = { enabled = true },
      },
    },
  },
}
vim.lsp.enable "pylsp"

-- Configure Ruff (Fast Linting & Formatting)
vim.lsp.config.ruff = {
  on_attach = function(client, bufnr)
    -- Disable hover capability in Ruff so pylsp handles hover/docstrings without UI flickering
    client.server_capabilities.hoverProvider = false
    nvconfig.on_attach(client, bufnr)
  end,
  on_init = nvconfig.on_init,
  capabilities = nvconfig.capabilities,
}
vim.lsp.enable "ruff"


-- Configure Assembly LSP (asm-lsp)
vim.lsp.config.asm_lsp = {
  on_attach = nvconfig.on_attach,
  on_init = nvconfig.on_init,
  capabilities = nvconfig.capabilities,
  filetypes = { "asm", "vmasm", "nasm", "s", "S" },
}
vim.lsp.enable "asm_lsp"
