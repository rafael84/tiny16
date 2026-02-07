-- tiny16se goto definition plugin
-- Simple implementation for jumping to function/constant definitions
-- Supports namespaced symbols (ns/symbol) from required modules
-- No external dependencies required

local M = {}

-- Cache for definitions in the current project
-- Structure: { symbol_name = { file = path, line = num, col = num } }
local definitions = {}

-- Cache for namespace mappings
-- Structure: { namespace_name = filepath }
local namespaces = {}

-- Track if full project has been indexed
local project_indexed = false

-- Parse a single file for definitions
local function parse_file(filepath)
	local file = io.open(filepath, "r")
	if not file then
		return
	end

	local current_ns = nil
	local line_num = 0

	for line in file:lines() do
		line_num = line_num + 1

		-- Match (ns namespace-name) - namespace declaration
		local ns_name = line:match("^%s*%(%s*ns%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if ns_name then
			current_ns = ns_name
			namespaces[ns_name] = filepath
		end

		-- Match (defn name ...)
		local defn_name = line:match("^%s*%(%s*defn%s+([a-zA-Z_%-][a-zA-Z0-9_%-!?]*)")
		if defn_name then
			local col = line:find(defn_name, 1, true) or 1
			-- Store both unqualified and qualified (ns/name) versions
			definitions[defn_name] = { file = filepath, line = line_num, col = col - 1 }
			if current_ns then
				definitions[current_ns .. "/" .. defn_name] = { file = filepath, line = line_num, col = col - 1 }
			end
		end

		-- Match (def NAME ...)
		local def_name = line:match("^%s*%(%s*def%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if def_name then
			local col = line:find(def_name, 1, true) or 1
			-- Store both unqualified and qualified (ns/name) versions
			definitions[def_name] = { file = filepath, line = line_num, col = col - 1 }
			if current_ns then
				definitions[current_ns .. "/" .. def_name] = { file = filepath, line = line_num, col = col - 1 }
			end
		end

		-- Match (defmacro name ...)
		local macro_name = line:match("^%s*%(%s*defmacro%s+([a-zA-Z_%-][a-zA-Z0-9_%-!?]*)")
		if macro_name then
			local col = line:find(macro_name, 1, true) or 1
			definitions[macro_name] = { file = filepath, line = line_num, col = col - 1 }
			if current_ns then
				definitions[current_ns .. "/" .. macro_name] = { file = filepath, line = line_num, col = col - 1 }
			end
		end

		-- Match (defrecord name ...)
		local record_name = line:match("^%s*%(%s*defrecord%s+([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if record_name then
			local col = line:find(record_name, 1, true) or 1
			definitions[record_name] = { file = filepath, line = line_num, col = col - 1 }
			if current_ns then
				definitions[current_ns .. "/" .. record_name] = { file = filepath, line = line_num, col = col - 1 }
			end
		end

		-- Match (var name ...) - global variables
		local var_name = line:match("^%s*%(%s*var%s+%^?[a-zA-Z0-9]*%s*([a-zA-Z_%-][a-zA-Z0-9_%-]*)")
		if var_name then
			local col = line:find(var_name, 1, true) or 1
			definitions[var_name] = { file = filepath, line = line_num, col = col - 1 }
			if current_ns then
				definitions[current_ns .. "/" .. var_name] = { file = filepath, line = line_num, col = col - 1 }
			end
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
	namespaces = {}
	local files = find_se_files()

	for _, file in ipairs(files) do
		parse_file(file)
	end

	project_indexed = true
	print(string.format("Indexed %d definitions (%d namespaces) from %d files", vim.tbl_count(definitions), vim.tbl_count(namespaces), #files))
end

-- Get the symbol under the cursor (supports namespaced symbols like ns/symbol)
local function get_symbol_under_cursor()
	local line = vim.api.nvim_get_current_line()
	local col = vim.api.nvim_win_get_cursor(0)[2] + 1

	-- Find word boundaries (tiny16se identifiers, including / for namespaced symbols)
	local start_col = col
	while start_col > 1 and line:sub(start_col - 1, start_col - 1):match("[a-zA-Z0-9_%-/]") do
		start_col = start_col - 1
	end

	local end_col = col
	while end_col <= #line and line:sub(end_col, end_col):match("[a-zA-Z0-9_%-/]") do
		end_col = end_col + 1
	end

	if start_col == end_col then
		return nil
	end

	local symbol = line:sub(start_col, end_col - 1)

	-- Verify it's a valid identifier (with optional namespace prefix)
	-- Matches: identifier OR namespace/identifier
	if symbol:match("^[a-zA-Z_%-][a-zA-Z0-9_%-]*$") or symbol:match("^[a-zA-Z_%-][a-zA-Z0-9_%-]*/[a-zA-Z_%-][a-zA-Z0-9_%-]*$") then
		return symbol
	end

	return nil
end

-- Jump to definition
function M.goto_definition()
	-- Index the project if not yet done
	if not project_indexed then
		M.index_project()
		project_indexed = true
	end

	local symbol = get_symbol_under_cursor()
	if not symbol then
		print("No symbol under cursor")
		return
	end

	-- First, check if it's a direct definition match
	local def = definitions[symbol]
	if def then
		vim.cmd(string.format("edit +%d %s", def.line, def.file))
		vim.api.nvim_win_set_cursor(0, { def.line, def.col })
		print(string.format("Jumped to definition of %s", symbol))
		return
	end

	-- If not found, check if it's a namespace name (e.g., from require statement)
	local ns_file = namespaces[symbol]
	if ns_file then
		vim.cmd(string.format("edit %s", ns_file))
		vim.api.nvim_win_set_cursor(0, { 1, 0 })
		print(string.format("Jumped to namespace %s", symbol))
		return
	end

	-- If it's a namespaced symbol that wasn't found, provide helpful message
	local ns, name = symbol:match("^([a-zA-Z_%-][a-zA-Z0-9_%-]*)/([a-zA-Z_%-][a-zA-Z0-9_%-]*)$")
	if ns then
		if namespaces[ns] then
			print(string.format("Symbol '%s' not found in namespace '%s' (%s)", name, ns, namespaces[ns]))
		else
			print(string.format("Namespace '%s' not found (symbol: %s)", ns, name))
		end
		return
	end

	print(string.format("Definition not found: %s", symbol))
end

-- Re-parse current file
function M.update_current_file()
	local filepath = vim.api.nvim_buf_get_name(0)
	if filepath:match("%.se$") then
		parse_file(filepath)
	end
end

-- List all indexed namespaces
function M.list_namespaces()
	if vim.tbl_count(namespaces) == 0 then
		M.index_project()
	end

	print("Indexed namespaces:")
	for ns, filepath in pairs(namespaces) do
		print(string.format("  %s -> %s", ns, filepath))
	end
end

-- Setup function to be called from init.lua
function M.setup()
	-- Create commands
	vim.api.nvim_create_user_command("Tiny16SeIndex", M.index_project, {})
	vim.api.nvim_create_user_command("Tiny16SeGoto", M.goto_definition, {})
	vim.api.nvim_create_user_command("Tiny16SeNamespaces", M.list_namespaces, {})

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
