return {
  {
    "stevearc/conform.nvim",
    opts = require "configs.conform",
  },

  {
    "neovim/nvim-lspconfig",
    config = function()
      require "configs.lspconfig"
    end,
  },

  {
    "mrcjkb/rustaceanvim",
    version = "^3",
    ft = { "rust" },
    config = function(_, _)
      vim.g.rustaceanvim = {
        server = {
          on_attach = function(client, buffer)
            require("core.utils").load_mappings("lspconfig", { buffer = buffer })
            require("nvchad.signature").setup(client)
          end,
        },
      }
    end,
  },

  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = {
        "vim",
        "lua",
        "vimdoc",
        "html",
        "css",
        "c",
        "cpp",
        "rust",
        "python",
        "asm"
      },
    },
  },
}
