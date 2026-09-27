-- This file needs to have same structure as nvconfig.lua 
-- https://github.com/NvChad/ui/blob/v3.0/lua/nvconfig.lua
-- Please read that file to know all available options :( 

---@class ChadrcConfig
local M = {}

M.base46 = {
    theme = "catppuccin",
  -- transparency = true,

  hl_override = {
    -- Highlight group colors
    ["@variable"] = { fg = "#F0323e" },
    ["@module"] = { fg = "#7aa2f7", bold = true, italic = true },

    -- Text formatting styles
    Comment = { italic = false },
    ["@comment"] = { italic = false },

    Keyword = { italic = true },
    ["@keyword"] = { italic = true },

    Function = { bold = true, italic = true },
    ["@function"] = { bold = true, italic = true },

    -- Python Highlights
    ["@variable.builtin"] = { fg = "#f7768e", italic = true }, -- self, cls
    ["@module.builtin"]   = { fg = "#7dcfff", italic = true }, -- os, sys, math
    ["@function.builtin"] = { fg = "#0db9d7", bold = true },   -- print(), len(), range()
    ["@variable.parameter"] = { fg = "#e0af68" },             -- function arguments

    -- C Highlights
    ["@variable.member"]   = { fg = "#73daca" },               -- struct fields (e.g. my_struct.field)
    ["@constant.macro"]    = { fg = "#ff9e64", bold = true },   -- UPPERCASE preprocessor macros
    ["@type"]              = { fg = "#2ac3de" },               -- typedefs and struct types


    ["@attribute.python"]            = { fg = "#bb9af7", italic = true }, -- Decorators (@classmethod, @property)
    ["@type.python"]                 = { fg = "#2ac3de", bold = true },   -- Classes & type hints (int, str, List)
    ["@exception.python"]            = { fg = "#f7768e", bold = true },   -- Exception keywords & classes (try, except, raise)
    ["@string.documentation.python"] = { fg = "#565f89", italic = true }, -- Multiline docstrings (""" ... """)
    ["@keyword.operator.python"]     = { fg = "#9d7cd8", bold = true },   -- Logical word operators (and, or, not, is, in)
    ["@property.python"]             = { fg = "#73daca" },                -- Object properties (self.attribute)

    -- Python LSP Semantic Tokens (Pyright / Basedpyright)
    ["@lsp.type.decorator.python"]   = { fg = "#b4f9f8" },
    ["@lsp.type.namespace.python"]   = { fg = "#7dcfff" },
    ["@lsp.type.parameter.python"]   = { fg = "#e0af68", italic = true },

    -- -------------------------------------------------------------------------
    -- C HIGHLIGHTS
    -- -------------------------------------------------------------------------
    ["@type.builtin.c"]              = { fg = "#2ac3de", bold = true },   -- Primitive types (int, char, double, void, size_t)
    ["@keyword.type.c"]              = { fg = "#bb9af7", italic = true }, -- Type declarations (struct, enum, union, typedef)
    ["@keyword.modifier.c"]          = { fg = "#9d7cd8", italic = true }, -- Storage modifiers (const, static, volatile, extern)
    ["@operator.c"]                  = { fg = "#89ddff" },                -- Pointer & struct ops (*, &, ->, .)
    ["@keyword.directive.c"]         = { fg = "#7dcfff", bold = true },   -- Directives (#include, #define, #pragma, #ifdef)
    ["@keyword.import.c"]            = { fg = "#9ece6a" },                -- Header file paths (<stdio.h>, "config.h")
    ["@constant.macro.c"]            = { fg = "#ff9e64", bold = true },   -- Preprocessor macros
    ["@property.c"]                  = { fg = "#73daca" },                -- Struct/union member variables
    ["@label.c"]                     = { fg = "#f7768e", bold = true },   -- Switch statement cases & goto labels

    -- C LSP Semantic Tokens (clangd)
    ["@lsp.type.enumMember.c"]       = { fg = "#ff9e64" },                -- Enum constant values
    ["@lsp.type.macro.c"]            = { fg = "#e0af68", bold = true },   -- Macro expansions
    ["@lsp.type.parameter.c"]        = { fg = "#e0af68", italic = true }, -- Function parameters


    -- -------------------------------------------------------------------------
    -- OPERATORS (+, -, *, /, =, ==, !=, +=, ->, &, etc.)
    -- -------------------------------------------------------------------------
    Operator                  = { fg = "#89ddff"}, -- Vim fallback
    ["@operator"]             = { fg = "#89ddff" }, -- General Treesitter operators
    ["@lsp.type.operator"]    = { fg = "#89ddff" }, -- LSP semantic token operators

    -- Language-specific operator overrides
    ["@operator.python"]      = { fg = "#89ddff" }, -- Python (+, *, =, in, is)
    ["@operator.c"]           = { fg = "#89ddff" }, -- C (+, *, &, ->, =)

    -- -------------------------------------------------------------------------
    -- OPTIONAL: PUNCTUATION & DELIMITERS
    -- -------------------------------------------------------------------------
    -- ["@punctuation.delimiter"] = { fg = "#7dcfff" }, -- Commas and semicolons (, ;)
    -- ["@punctuation.bracket"]   = { fg = "#bb9af7" }, -- Brackets and parens (( ), { }, [ ])
    ["@punctuation.special"]   = { fg = "#89ddff" }, -- String formatting operators (%s, f"{}")
  },
}

M.nvdash = { load_on_startup = true }
M.ui = {
      tabufline = {
         lazyload = true,
         treeOffsetFt = "NvimTree",
         order = {"treeOffset","buffers","tabs"},
     },
     cmp = {
       lspkind_text = true,
       style = "atom",
     },
}

M.colorify = {
    enabled = true,
    mode = "bg",
}

M.lsp = { signature = true }

return M
