-- neon-void (https://github.com/RoopakShukla/neon-void)
-- by Roopak Shukla

local lexers = vis.lexers

local colors = {
    ['bg']          = '0',
    ['bg_alt']      = '236',
    ['selection']   = '239',
    ['comment']     = '244',
    ['fg']          = '255',
    ['red']         = '196',
    ['orange']      = '208',
    ['yellow']      = '220',
    ['green']       = '82',
    ['cyan']        = '51',
    ['blue']        = '39',
    ['purple']      = '141',
    ['magenta']     = '201',
}

lexers.STYLE_DEFAULT = 'fore:'..colors.fg..',back:'..colors.bg
lexers.STYLE_NOTHING = 'back:'..colors.bg
lexers.STYLE_CLASS = 'fore:'..colors.yellow
lexers.STYLE_COMMENT = 'fore:'..colors.comment..',italics'
lexers.STYLE_CONSTANT = 'fore:'..colors.orange
lexers.STYLE_DEFINITION = 'fore:'..colors.purple
lexers.STYLE_ERROR = 'fore:'..colors.red..',italics'
lexers.STYLE_FUNCTION = 'fore:'..colors.blue..',italics'
lexers.STYLE_KEYWORD = 'fore:'..colors.purple..',bold'
lexers.STYLE_LABEL = 'fore:'..colors.yellow
lexers.STYLE_NUMBER = 'fore:'..colors.orange
lexers.STYLE_OPERATOR = 'fore:205'
lexers.STYLE_REGEX = 'fore:'..colors.cyan
lexers.STYLE_STRING = 'fore:'..colors.green
lexers.STYLE_PREPROCESSOR = 'fore:'..colors.red..',italics'
lexers.STYLE_TAG = 'fore:'..colors.magenta
lexers.STYLE_TYPE = 'fore:'..colors.yellow..',bold,italics'
lexers.STYLE_VARIABLE = 'fore:'..colors.blue
lexers.STYLE_WHITESPACE = 'fore:'..colors.bg_alt
lexers.STYLE_EMBEDDED = 'fore:'..colors.magenta
lexers.STYLE_IDENTIFIER = 'fore:'..colors.cyan

lexers.STYLE_LINENUMBER = 'fore:'..colors.comment..',back:'..colors.bg
lexers.STYLE_LINENUMBER_CURSOR = 'fore:'..colors.yellow..',back:'..colors.bg..',bold'

--            Reverse the Colors          --
-- lexers.STYLE_CURSOR = 'reverse'
-- lexers.STYLE_CURSOR_PRIMARY = 'reverse'

--               OR               -- 

--           CLASSIC              --
lexers.STYLE_CURSOR = 'fore:'..colors.bg..',back:'..colors.fg
lexers.STYLE_CURSOR_PRIMARY = 'fore:'..colors.bg..',back:'..colors.fg


--           You can set a custom color of cursor here        --

-- lexers.STYLE_CURSOR = 'fore:0,back:214'
-- lexers.STYLE_CURSOR_PRIMARY = 'fore:0,back:214'

lexers.STYLE_CURSOR_LINE = 'back:'..colors.bg_alt
lexers.STYLE_COLOR_COLUMN = 'back:'..colors.bg_alt
lexers.STYLE_SELECTION = 'back:'..colors.selection
lexers.STYLE_STATUS = 'fore:'..colors.fg..',back:'..colors.bg_alt
lexers.STYLE_STATUS_FOCUSED = 'fore:'..colors.orange..',back:'..colors.bg_alt
lexers.STYLE_SEPARATOR = lexers.STYLE_DEFAULT
lexers.STYLE_INFO = 'fore:default,back:default,bold'
lexers.STYLE_EOF = 'back:'..colors.bg
