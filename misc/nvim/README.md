# tiny16 Neovim Support

Syntax highlighting and go-to-definition for tiny16 assembly (`.asm`) and tiny16 SE (`.se`) files.

## Features

- **Syntax Highlighting**: Full syntax support for both `.asm` and `.se` files
- **Go to Definition**: Jump to function/constant/data definitions in `.se` files (press `gd`)
- **Zero Dependencies**: Pure Lua/Vimscript implementation

## Installation

### Using lazy.nvim

```lua
{
  dir = "/path/to/tiny16/misc/nvim",  -- Update this path
  ft = { "tiny16asm", "tiny16se" },
  config = function()
    require("tiny16se-goto").setup()
  end,
}
```

### Manual Installation

```bash
# Copy files to your Neovim config directory
mkdir -p ~/.config/nvim/{syntax,ftdetect,lua}
cp misc/nvim/syntax/*.vim ~/.config/nvim/syntax/
cp misc/nvim/ftdetect/tiny16.vim ~/.config/nvim/ftdetect/
cp misc/nvim/lua/tiny16se-goto.lua ~/.config/nvim/lua/
```

Then add to your `init.lua`:

```lua
require("tiny16se-goto").setup()
```

## Filetype Detection

- `.se` files are automatically detected as `tiny16se`
- `.asm` files are detected as `tiny16asm` if they contain tiny16-specific patterns:
  - Include statements referencing `tiny16.inc` or stdlib
  - tiny16-specific instructions like `LOADI`, `MOVSPR`, `MOVRSP`
  - Comments starting with `; tiny16`

### Manual Override

You can force the filetype with a modeline at the end of your file:

```asm
; vim: ft=tiny16asm
```

```scheme
; vim: ft=tiny16se
```

Or set it manually in Neovim:

```vim
:set filetype=tiny16asm
:set filetype=tiny16se
```

## Usage

### Go to Definition (tiny16se only)

When editing a `.se` file:

1. Place cursor on any symbol (function, constant, or data label)
2. Press `gd` to jump to its definition
3. Press `<C-o>` to jump back

Available commands:

- `:Tiny16SeIndex` - Re-index all definitions in project
- `:Tiny16SeGoto` - Jump to definition under cursor

### Syntax Highlighting

#### Assembly (`tiny16asm`)

- Instructions: `LOADI`, `LOAD`, `STORE`, `MOV`, `ADD`, `SUB`, `JMP`, `CALL`, etc.
- Registers: `R0`-`R7`, `SP`, `PC`, `FP`, register pairs (`R6:R7`)
- Directives: `.macro`, `.endmacro`, `.include`, `section`, `ORG`, `TIMES`, `DB`
- Labels and constants
- Numbers: decimal, hexadecimal (`0x`), binary (`0b`)
- Strings with escape sequences
- Expression operators
- Common stdlib macros

#### SE Language (`tiny16se`)

- Special forms: `def`, `defn`, `let`, `set`, `if`, `while`, `do`, `data`, `db`, `repeat`
- Primitives: arithmetic, bitwise, comparison, memory operations
- Operator aliases: `+`, `-`, `&`, `|`, `^`, `~`, `<<`, `>>`, `=`, `!=`, `<`, `>`, `<=`, `>=`
- Function and constant definitions
- Numbers and strings
- S-expression parentheses

## Customization

The syntax files use standard Vim highlight groups. Customize colors in your Neovim config:

```lua
-- Example: Make instructions bold
vim.api.nvim_set_hl(0, "tiny16asmInstr", { link = "Statement", bold = true })

-- Example: Different color for registers
vim.api.nvim_set_hl(0, "tiny16asmRegister", { fg = "#ff9900" })
```
