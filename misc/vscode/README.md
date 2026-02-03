# tiny16 VSCode/Cursor Extension

Syntax highlighting for tiny16 assembly (`.asm`) and tiny16 SE (`.se`) files in VSCode and Cursor.

## Features

- **Syntax Highlighting**: Full syntax support for both `.asm` and `.se` files
- **Auto-completion**: Bracket and quote pairing
- **Comment Toggle**: Use `Cmd+/` (Mac) or `Ctrl+/` (Windows/Linux) to toggle comments
- **Code Folding**: Fold `.macro` / `.endmacro` blocks in assembly files

## Installation

### Method 1: Install as Local Extension (Recommended)

1. Copy the extension to your VSCode/Cursor extensions folder:

```bash
# For VSCode
mkdir -p ~/.vscode/extensions/tiny16-syntax
cp -r misc/vscode/* ~/.vscode/extensions/tiny16-syntax/

# For Cursor
mkdir -p ~/.cursor/extensions/tiny16-syntax
cp -r misc/vscode/* ~/.cursor/extensions/tiny16-syntax/
```

2. Restart VSCode/Cursor
3. Open any `.asm` or `.se` file to see syntax highlighting

### Method 2: Use from Workspace (Development)

For testing or development, you can load the extension directly:

1. Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
2. Type "Developer: Install Extension from Location"
3. Navigate to `/path/to/tiny16/misc/vscode`
4. Select the folder

### Method 3: Symlink (Auto-update)

Create a symlink so changes are reflected automatically:

```bash
# For VSCode
ln -s "$(pwd)/misc/vscode" ~/.vscode/extensions/tiny16-syntax

# For Cursor
ln -s "$(pwd)/misc/vscode" ~/.cursor/extensions/tiny16-syntax
```

## Syntax Highlighting

### Assembly (`.asm` files)

- **Instructions**: Data movement, arithmetic, bitwise, comparison, control flow, stack operations
- **Registers**: `R0`-`R7`, `SP`, `PC`, `FP`, register pairs (`R6:R7`)
- **Directives**: `.macro`, `.endmacro`, `.include`, `section`, `ORG`, `TIMES`, `DB`
- **Sections**: `.code`, `.data`
- **Labels**: Function/jump labels with `:` suffix
- **Constants**: Uppercase names with `=` assignment
- **Numbers**: Decimal, hexadecimal (`0x`), binary (`0b`)
- **Strings**: With escape sequences (`\n`, `\r`, `\t`, etc.)
- **Macros**: Common stdlib macros (SETADDR, LOAD16, PUSH2, etc.)
- **Comments**: Lines starting with `;`

### SE Language (`.se` files)

- **Special Forms**: `def`, `defn`, `let`, `set`, `if`, `while`, `do`, `data`, `db`, `repeat`, `include`
- **Builtins**: Arithmetic (`add`, `sub`, `inc`, `dec`), bitwise (`and`, `or`, `xor`, `not`, `shl`, `shr`), comparison (`eq`, `ne`, `lt`, `gt`, `le`, `ge`)
- **Operators**: `+`, `-`, `*`, `&`, `|`, `^`, `~`, `<<`, `>>`, `=`, `!=`, `<`, `>`, `<=`, `>=`, `!`
- **Memory Operations**: `load`, `store`, `addr`, `peek`, `poke`, `peek16`, `poke16`
- **Function Definitions**: Highlighted function names after `defn`
- **Constants**: Highlighted constant names after `def`
- **Data Labels**: Highlighted data labels after `data`
- **Booleans**: `true`, `false`
- **Comments**: Lines starting with `;`

## Customization

You can customize colors in your VSCode/Cursor `settings.json`:

```json
{
  "editor.tokenColorCustomizations": {
    "textMateRules": [
      {
        "scope": "keyword.control.tiny16asm",
        "settings": {
          "foreground": "#569cd6",
          "fontStyle": "bold"
        }
      },
      {
        "scope": "variable.other.register.tiny16asm",
        "settings": {
          "foreground": "#9cdcfe"
        }
      },
      {
        "scope": "entity.name.function.tiny16se",
        "settings": {
          "foreground": "#dcdcaa",
          "fontStyle": "bold"
        }
      }
    ]
  }
}
```

## Language Configuration

Both languages support:

- Line comments with `;`
- Auto-closing brackets and quotes
- Bracket matching and highlighting

## File Association

The extension automatically applies syntax highlighting to:

- `.asm` files → tiny16 Assembly
- `.se` files → tiny16 SE

To manually set the language for a file:

1. Click the language indicator in the bottom-right corner of VSCode/Cursor
2. Select "tiny16 Assembly" or "tiny16 SE" from the list

Or add a comment at the top of your file:

```
; Language: tiny16asm
```

## Troubleshooting

### Syntax highlighting not working

1. Make sure you've restarted VSCode/Cursor after installation
2. Check that the file extension is `.asm` or `.se`
3. Manually select the language from the status bar
4. Run "Developer: Reload Window" from the command palette

### Colors don't match my theme

The extension uses semantic token scopes that work with most themes. If colors don't look right:

1. Try a different theme
2. Customize colors using `editor.tokenColorCustomizations` (see Customization section)

## Related

See also the Neovim/Vim syntax files in `misc/nvim/` for terminal-based editing.
