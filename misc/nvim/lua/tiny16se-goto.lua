-- tiny16se goto definition plugin
-- Simple implementation for jumping to function/constant definitions
-- No external dependencies required

local M = {}

-- Cache for definitions in the current project
-- Structure: { symbol_name = { file = path, line = num, col = num } }
local definitions = {}

-- Parse a single file for definitions
local function parse_file(filepath)
	local file = io.open(filepath, "r")
	if not file then
		return
	end

	local line_num = 0
	for line in file:lines() do
		line_num = line_num + 1

		-- Match (defn name ...)
		local defn_name = line:match("^%s*%(%s*defn%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if defn_name then
			local col = line:find(defn_name, 1, true) or 1
			definitions[defn_name] = { file = filepath, line = line_num, col = col - 1 }
		end

		-- Match (def NAME ...)
		local def_name = line:match("^%s*%(%s*def%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if def_name then
			local col = line:find(def_name, 1, true) or 1
			definitions[def_name] = { file = filepath, line = line_num, col = col - 1 }
		end

		-- Match (data name ...)
		local data_name = line:match("^%s*%(%s*data%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if data_name then
			local col = line:find(data_name, 1, true) or 1
			definitions[data_name] = { file = filepath, line = line_num, col = col - 1 }
		end
	end

	file:close()
end

-- Find all .se files in the project
local function find_se_files()
	local files = {}

	-- Get project root (where .git is)
	local root = vim.fn.finddir(".git", ".;")
	if root == "" then
		-- No git repo, use current directory
		root = "."
	else
		root = vim.fn.fnamemodify(root, ":h")
	end

	-- Find all .se files using vim.fn.glob
	local pattern = root .. "/**/*.se"
	local found = vim.fn.glob(pattern, false, true)

	for _, file in ipairs(found) do
		table.insert(files, file)
	end

	return files
end

-- Index all .se files in the project
function M.index_project()
	definitions = {}
	local files = find_se_files()

	for _, file in ipairs(files) do
		parse_file(file)
	end

	print(string.format("Indexed %d definitions from %d files", vim.tbl_count(definitions), #files))
end

-- Get the symbol under the cursor
local function get_symbol_under_cursor()
	local line = vim.api.nvim_get_current_line()
	local col = vim.api.nvim_win_get_cursor(0)[2] + 1

	-- Find word boundaries (tiny16se identifiers)
	local start_col = col
	while start_col > 1 and line:sub(start_col - 1, start_col - 1):match("[a-zA-Z0-9_%-]") do
		start_col = start_col - 1
	end

	local end_col = col
	while end_col <= #line and line:sub(end_col, end_col):match("[a-zA-Z0-9_%-]") do
		end_col = end_col + 1
	end

	if start_col == end_col then
		return nil
	end

	local symbol = line:sub(start_col, end_col - 1)

	-- Verify it's a valid identifier
	if symbol:match("^[a-zA-Z_%-][a-zA-Z0-9_%-]*$") then
		return symbol
	end

	return nil
end

-- Jump to definition
function M.goto_definition()
	-- Index the project if definitions are empty
	if vim.tbl_count(definitions) == 0 then
		M.index_project()
	end

	local symbol = get_symbol_under_cursor()
	if not symbol then
		print("No symbol under cursor")
		return
	end

	local def = definitions[symbol]
	if not def then
		print(string.format("Definition not found: %s", symbol))
		return
	end

	-- Jump to the definition
	vim.cmd(string.format("edit +%d %s", def.line, def.file))
	vim.api.nvim_win_set_cursor(0, { def.line, def.col })
	print(string.format("Jumped to definition of %s", symbol))
end

-- Re-parse current file
function M.update_current_file()
	local filepath = vim.api.nvim_buf_get_name(0)
	if filepath:match("%.se$") then
		parse_file(filepath)
	end
end

-- Setup function to be called from init.lua
function M.setup()
	-- Create commands
	vim.api.nvim_create_user_command("Tiny16SeIndex", M.index_project, {})
	vim.api.nvim_create_user_command("Tiny16SeGoto", M.goto_definition, {})

	-- Auto-index on enter and after save
	vim.api.nvim_create_autocmd("BufRead", {
		pattern = "*.se",
		callback = function()
			M.update_current_file()
		end,
	})

	vim.api.nvim_create_autocmd("BufWritePost", {
		pattern = "*.se",
		callback = function()
			M.update_current_file()
		end,
	})

	-- Set up keybindings for .se files
	vim.api.nvim_create_autocmd("FileType", {
		pattern = "tiny16se",
		callback = function()
			vim.keymap.set("n", "gd", M.goto_definition, { buffer = true, silent = true, desc = "Go to definition" })
			vim.keymap.set("n", "<C-]>", M.goto_definition, { buffer = true, silent = true, desc = "Go to definition" })
		end,
	})
end

return M
