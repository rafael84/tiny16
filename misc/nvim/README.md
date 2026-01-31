# tiny16 Neovim Syntax Highlighting

Syntax highlighting for tiny16 assembly (`.asm`) and tiny16 SE (`.se`) files.

## Installation

### Option 1: Copy to Neovim config

Copy the contents to your Neovim configuration directory:

```bash
# Create directories if they don't exist
mkdir -p ~/.config/nvim/syntax
mkdir -p ~/.config/nvim/ftdetect

# Copy syntax files
cp syntax/tiny16asm.vim ~/.config/nvim/syntax/
cp syntax/tiny16se.vim ~/.config/nvim/syntax/

# Copy filetype detection
cp ftdetect/tiny16.vim ~/.config/nvim/ftdetect/
```

### Option 2: Add as a local plugin (lazy.nvim)

Add to your `lazy.nvim` config:

```lua
{
  dir = "/path/to/tiny16/misc/nvim",
  ft = { "tiny16asm", "tiny16se" },
}
```

### Option 3: Symlink (development)

```bash
ln -s /path/to/tiny16/misc/nvim/syntax/tiny16asm.vim ~/.config/nvim/syntax/
ln -s /path/to/tiny16/misc/nvim/syntax/tiny16se.vim ~/.config/nvim/syntax/
ln -s /path/to/tiny16/misc/nvim/ftdetect/tiny16.vim ~/.config/nvim/ftdetect/
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

## Features

### Assembly (`tiny16asm`)

- Instructions: `LOADI`, `LOAD`, `STORE`, `MOV`, `ADD`, `SUB`, `JMP`, `CALL`, etc.
- Registers: `R0`-`R7`, `SP`, `PC`, `FP`, register pairs (`R6:R7`)
- Directives: `.macro`, `.endmacro`, `.include`, `section`, `ORG`, `TIMES`, `DB`
- Labels and constants
- Numbers: decimal, hexadecimal (`0x`), binary (`0b`)
- Strings with escape sequences
- Expression operators
- Common stdlib macros

### SE Language (`tiny16se`)

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
