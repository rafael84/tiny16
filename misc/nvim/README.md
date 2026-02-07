# tiny16 Neovim Support

Syntax highlighting and go-to-definition for tiny16 assembly (`.asm`) and tiny16 SE (`.se`) files.

## Features

- **Syntax Highlighting**: Full syntax support for both `.asm` and `.se` files
- **Go to Definition**: Jump to function/constant/variable/record/macro/data definitions in `.se` files (press `gd`)
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

### Using Makefile (symlinks)

```bash
cd misc/nvim
make install    # Creates symlinks to ~/.config/nvim
make uninstall  # Removes symlinks
```

Then add to your `init.lua`:

```lua
require("tiny16se-goto").setup()
```

### Manual Installation (copy)

```bash
mkdir -p ~/.config/nvim/{syntax,ftdetect,ftplugin,lua}
cp misc/nvim/syntax/*.vim ~/.config/nvim/syntax/
cp misc/nvim/ftdetect/tiny16.vim ~/.config/nvim/ftdetect/
cp misc/nvim/ftplugin/tiny16se.vim ~/.config/nvim/ftplugin/
cp misc/nvim/lua/tiny16se-goto.lua ~/.config/nvim/lua/
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

1. Place cursor on any symbol (function, constant, variable, record, macro, or data label)
2. Press `gd` to jump to its definition
3. Press `<C-o>` to jump back

**Namespace Support**: The plugin fully supports namespaced symbols:

- Jump to `apu/NOTE_C3` will find `NOTE_C3` in the file declaring `(ns apu)`
- Jump to a namespace name (e.g., `apu` in a `require` statement) opens the namespace file
- Both qualified (`apu/init`) and unqualified (`init`) lookups are supported

Available commands:

- `:Tiny16SeIndex` - Re-index all definitions in project
- `:Tiny16SeGoto` - Jump to definition under cursor
- `:Tiny16SeNamespaces` - List all indexed namespaces and their files

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

- Special forms: `def`, `defn`, `defmacro`, `defrecord`, `var`, `let`, `fn`, `data`, `set!`
- Control flow: `if`, `cond`, `when`, `unless`, `while`, `for`, `do`
- Module system: `ns`, `require`, `import`
- Logic: `and`, `or`, `not` (short-circuit)
- Builtins: `inc`, `dec`, `neg`, `load`, `store`, `hi`, `lo`, `nth`, `len`, `array`, `range`, `db`, `asm`
- Word-form aliases: `add`, `sub`, `mul`, `div`, `mod`, `eq`, `ne`, `lt`, `gt`, `le`, `ge`, `xor`, `shl`, `shr`, `lnot`
- Type predicates: `nil?`, `zero?`, `pos?`, `neg?`
- Type casts: `u8`, `i8`
- Operators: `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `~`, `<<`, `>>`, `=`, `!=`, `<`, `>`, `<=`, `>=`, `!`
- Keywords: `:alive`, `:dead`, `:x`, etc.
- Type hints: `^u8`, `^i8`, `^u16`, `^i16`
- Special literals: `nil`, `true`, `false`
- Numbers: decimal, hexadecimal, negative, fixed-point (8.8)
- Strings with escape sequences
- Namespaced symbols: `apu/init`, `sprite/draw`
- S-expression parentheses
- Lisp-style indentation (2 spaces)

## Customization

### Indentation

The SE filetype plugin enables Lisp-style indentation by default with these settings:

- 2-space indentation
- `lisp` mode for S-expression aware indentation
- Keywords like `def`, `defn`, `defmacro`, `defrecord`, `var`, `let`, `fn`, `data`, `if`, `cond`, `when`, `unless`, `while`, `for`, `do`, `ns`, `require`, `import` use body indentation

Override in your config if needed:

```lua
vim.api.nvim_create_autocmd("FileType", {
  pattern = "tiny16se",
  callback = function()
    vim.opt_local.shiftwidth = 4  -- Use 4-space indentation
    vim.opt_local.tabstop = 4
  end
})
```

### Syntax Highlighting

The syntax files use standard Vim highlight groups. Customize colors in your Neovim config:

```lua
-- Example: Make instructions bold
vim.api.nvim_set_hl(0, "tiny16asmInstr", { link = "Statement", bold = true })

-- Example: Different color for registers
vim.api.nvim_set_hl(0, "tiny16asmRegister", { fg = "#ff9900" })
```
