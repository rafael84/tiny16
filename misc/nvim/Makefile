NVIM_CONFIG := $(HOME)/.config/nvim
PLUGIN_DIR  := $(shell cd "$(dir $(lastword $(MAKEFILE_LIST)))" && pwd)

.PHONY: install uninstall help

install:
	@mkdir -p $(NVIM_CONFIG)/syntax
	@mkdir -p $(NVIM_CONFIG)/ftdetect
	@mkdir -p $(NVIM_CONFIG)/ftplugin
	@mkdir -p $(NVIM_CONFIG)/lua
	@ln -sf $(PLUGIN_DIR)/syntax/tiny16asm.vim $(NVIM_CONFIG)/syntax/
	@ln -sf $(PLUGIN_DIR)/syntax/tiny16se.vim $(NVIM_CONFIG)/syntax/
	@ln -sf $(PLUGIN_DIR)/ftdetect/tiny16.vim $(NVIM_CONFIG)/ftdetect/
	@ln -sf $(PLUGIN_DIR)/ftplugin/tiny16se.vim $(NVIM_CONFIG)/ftplugin/
	@ln -sf $(PLUGIN_DIR)/lua/tiny16se-goto.lua $(NVIM_CONFIG)/lua/
	@echo "Done! Add to your init.lua:"
	@echo '  require("tiny16se-goto").setup()'

uninstall:
	@rm -f $(NVIM_CONFIG)/syntax/tiny16asm.vim
	@rm -f $(NVIM_CONFIG)/syntax/tiny16se.vim
	@rm -f $(NVIM_CONFIG)/ftdetect/tiny16.vim
	@rm -f $(NVIM_CONFIG)/ftplugin/tiny16se.vim
	@rm -f $(NVIM_CONFIG)/lua/tiny16se-goto.lua
