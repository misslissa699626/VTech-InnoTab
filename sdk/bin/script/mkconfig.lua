require "common"
lfs = require "lfs"

---------------------------------------------------------------------
-- Config utility
---------------------------------------------------------------------
function configToString(config)
	local s = StringBuffer:New()
	for k, v in pairsBySortedKey(config) do
		if k:sub(1, 1) ~= "_" then
			if type(v) == "boolean" then
				if v == true then
					s:WriteFormat("SYSCONFIG_%s = y\n", k)
				else
					s:WriteFormat("SYSCONFIG_%s = n\n", k)
				end
			else
				--s:WriteFormat("SYSCONFIG_%s = \"%s\"\n", k, v)
				s:WriteFormat("SYSCONFIG_%s = %s\n", k, v)
			end
		end
	end
	return s:ToString()
end

function configToHeaderFormat(config)
	local s = StringBuffer:New()
	s:WriteFormat("#ifndef _MKCONFIG_\n")
	s:WriteFormat("#define _MKCONFIG_\n")
	s:WriteFormat("\n")
	for k, v in pairsBySortedKey(config) do
		if k:sub(1, 1) == "_" then
			-- skip
		elseif (v == true) then
			s:WriteFormat("#define SYSCONFIG_%s\n\n", k)
		elseif (v == false) then
			s:WriteFormat("/* #undef SYSCONFIG_%s */\n\n", k)
		elseif (v == "y") then
			s:WriteFormat("#define SYSCONFIG_%s\n\n", k)
		elseif (v == "n") then
			s:WriteFormat("/* #undef SYSCONFIG_%s */\n\n", k)
		elseif (type(v) == "number") then
			s:WriteFormat("#define SYSCONFIG_%s %d\n\n", k, v)
		else
			s:WriteFormat("#define SYSCONFIG_%s \"%s\"\n\n", k, v)
		end
	end
	s:WriteFormat("\n")
	s:WriteFormat("#endif\n")
	return s:ToString()
end

function configToLuaFormat(config)
	local s = StringBuffer:New()
	s:WriteFormat("local sysconfig = {\n")
	for k, v in pairsBySortedKey(config) do
		if k:sub(1, 1) == "_" then
			-- skip
		elseif (type(v) == "boolean") then
			s:WriteFormat("\t[%q] = %s,\n", k, tostring(v))
		elseif (type(v) == "number") then
			s:WriteFormat("\t[%q] = %s,\n", k, tostring(v))
		else
			s:WriteFormat("\t[%q] = %q,\n", k, tostring(v))
		end
	end
	s:WriteFormat("}\n")
	s:WriteFormat("return sysconfig\n")
	return s:ToString()
end

function dumpConfig(config)
	printf("=== Summary ===\n")
	printf(configToString(config))
	printf("\n");
end


function saveConfig(filename, config)
	local f = assert(io.open(filename, "wb"));
	fprintf(f, "#=== Project Configuration ===\n\n")
	f:write(configToString(config))
	fprintf(f, "\n");
	f:close();
end


function saveToFile(filename, str)
	local f = assert(io.open(filename, "wb"));
	f:write(str)
	f:close();
end

---------------------------------------------------------------------
-- Menu utility
---------------------------------------------------------------------
function selectList(title, list, valueList)
	local n = 0
	local valid = true
	valueList = valueList or list
	local nlist = {}

	for i = 1, #list do
		table.insert(nlist, i, tostring(i))
	end
	--printf("nlist = << %s >>\n", nlist)

	repeat
		valid = true
		printf("<< %s >>\n", title)
		for i = 1, #list do
			if #list >= 10 then
				printf("%2d. %s\n", i, list[i])
			else
				printf("%d. %s\n", i, list[i])
			end
		end
		printf("Select [1 .. %d] :", #list)

		local line = io.read("*line")
		--printf("line = << %s >>\n", line)
		n = selectToNumber(nlist, line)
		if (n == 0) then
			printf("\n");
			printf("Error : invalid value!\n\n");
			valid = false
		end
	until (valid == true)
	printf("\n")

	return valueList[n], n
end

function selectBool(title, message)
	local n = 0
	local valid = true
	valueList = { "y", "n" }
	local nlist = {}

	table.insert(nlist, 1, "y")
	table.insert(nlist, 2, "n")

	repeat
		valid = true
		printf("<< %s >>\n", title)
		printf("%s [y/n] :", message)

		local line = io.read("*line")
		n = selectToNumber(nlist, line)
		if (n == 0) then
			printf("\n");
			printf("Error : invalid value!\n\n");
			valid = false
		end
	until (valid == true)
	printf("\n")

	if valueList[n] == "y" then
		return true
	else
		return false
	end
end

function selectToNumber(list, select)
	if (select == nil) then
		return 0
	end

	for i = 1, #list do
		if list[i] == select then
			return i
		end
	end
	return 0
end

function selectConfigList(title, list, valueList)
	local n = 0
	local valid = true
	valueList = valueList or list
	local nlist = {}

	for i = 1, #list do
		table.insert(nlist, i, tostring(i))
	end
	--printf("nlist = << %s >>\n", nlist)

	repeat
		valid = true
		printf("<< %s >>\n", title)
		for i = 1, #list do
			printf("%d. %s\n", i, list[i])
		end
		printf("Select [1 .. %d] :", #list)

		local line = io.read("*line")
		--printf("line = << %s >>\n", line)
		n = selectToNumber(nlist, line)
		if (n == 0) then
			printf("\n");
			printf("Error : invalid value!\n\n");
			valid = false
		end
	until (valid == true)
	printf("\n")

	return valueList[n]
end


---------------------------------------------------------------------
-- Import configs
---------------------------------------------------------------------
CURR_DIR = "."
CONFIG = {}
config = {}

function prompt(args)
	return { key = "prompt", value = args[1] }
end

function conf(args)
	return { key = "conf", value = args[1] }
end

function path(args)
	return { key = "path", value = args[1] }
end

function resolution(args)
	return { key = "resolution", value = args[1] }
end

function physical(args)
	return { key = "physical", value = args[1] }
end

function IncludeConfig(filename)
	assert(loadfile(filename))(config, CONFIG)
end

function parseConfigList(conf)
	local promptList = {}
	local confList = {}
	local pathList = {}
	for i, item in ipairs(conf) do
		for j, v in ipairs(item) do
			item[v.key] = v.value
		end
		promptList[i] = item.prompt
		confList[i] = item.conf
		pathList[i] = item.path
	end
	return promptList, confList, pathList
end

function parseDisplayList(conf)
	local promptList = {}
	local confList = {}
	local resolutionList = {}
	local physicalResList = {}
	for i, item in ipairs(conf) do
		for j, v in ipairs(item) do
			item[v.key] = v.value
		end
		promptList[i] = string.format("%s - %s", item.resolution or "N/A", item.prompt or "N/A")
		confList[i] = item.conf
		resolutionList[i] = item.resolution
		physicalResList[i] = item.physical
	end
	return promptList, confList, resolutionList, physicalResList
end

function indexOfTable(t, a)
	for i = 1, #t do
		if t[i] == a then
			return i
		end
	end
	return 0
end

function findMetaObjects(list, path, top, metaFilename)
	for dname in lfs.dir(path) do
		if dname:sub(1,1) ~= "." then
			local dirname = path .. "/" .. dname
			local attr = lfs.attributes(dirname)
			if attr and attr.mode == "directory" then
				local metaFile = dirname .. "/" .. metaFilename
				local metaFunc = loadfile(metaFile)
				if metaFunc then
					local meta = metaFunc()
					meta.path = dirname
					meta.shortname = dname
					meta.subpath = string.match(dirname, top .. "/(.*)")
					meta.name = string.gsub(meta.subpath, "/", ".")
					list[meta.name] = meta
				else
					-- recursive find
					findMetaObjects(list, dirname, top, metaFilename)
				end
			end
		end
	end
end

function findProjects(path)
	local list = {}
	findMetaObjects(list, path, path, "project.meta")
	return list
end

function findPlatforms(path)
	local list = {}
	findMetaObjects(list, path, path, "platform.meta")
	return list
end

function selectMetaObjects(list, prompt)
	local prompts = {}
	local sortedList = {}
	for k, v in pairsBySortedKey(list) do
		prompts[#prompts + 1] = v.name .. " // " .. v.prompt
		sortedList[#sortedList + 1] = v
	end
	local object = selectList(prompt, prompts, sortedList)
	return object
end
	
function selectProject(list)
	return selectMetaObjects(list, "Select Project")
end

function selectPlatform(list)
	return selectMetaObjects(list, "Select Platform")
end

function findFiles(path, pattern, depth, list)
	list = list or {}
	for filename in lfs.dir(path) do
		if filename ~= "." and filename ~= ".." then
			local pathname = path .. "/" .. filename
			local attr = lfs.attributes(pathname)

			if attr and attr.mode == "file" then
				local capture = string.match(filename, pattern)
				if capture then
					--print("pathname:", pathname)
					list[pathname] = capture
				end		
			elseif attr and attr.mode == "directory" then
				-- recursive find
				if depth ~= 0 then
					findFiles(pathname, pattern, depth - 1, list)
				end
			end
		end
	end
	return list
end

function loadProjectConfig(pathname)
	local t = {}
	local subpath, basename = string.match(pathname, "^project/(.*)/([^/]*)$")

	t.cfgpath = pathname
	t.path = "project/" .. subpath
	t.name = string.gsub(subpath, "%/", ".")
	t.filename = basename
	--print(t.path, t.name, t.filename)
	t.meta = assert(loadfile(t.path .. "/project.meta")())
	
	return t
end

function findProjectConfigs(path)
	local list = findFiles(path, "^project(.*)%.cfg$", 4)
	local t = {}
	for k, v in pairsBySortedKey(list) do
		t[#t+1] = loadProjectConfig(k)
	end
	return t
end

function selectProjectConfig(list)
	local prompts = {}
	local sortedList = {}
	for i, v in ipairs(list) do
		prompts[#prompts + 1] = v.cfgpath .. "  // " .. v.meta.prompt
		sortedList[#sortedList + 1] = v
	end
	local object = selectList("Select Project", prompts, sortedList)
	return object
end

function loadPlatformConfig(pathname)
	local t = {}
	local subpath, basename = string.match(pathname, "^platform/(.*)/([^/]*)$")

	t.cfgpath = pathname
	t.path = "platform/" .. subpath
	t.name = string.gsub(subpath, "%/", ".")
	t.filename = basename
	--print(t.path, t.name, t.filename)
	t.meta = assert(loadfile(t.path .. "/platform.meta")())
	
	return t
end

function findPlatformConfigs(path)
	local list = findFiles(path, "^platform(.*)%.cfg$", 4)
	local t = {}
	for k, v in pairsBySortedKey(list) do
		t[#t+1] = loadPlatformConfig(k)
	end
	return t
end

function selectPlatformConfig(list)
	local prompts = {}
	local sortedList = {}
	for i, v in ipairs(list) do
		prompts[#prompts + 1] = v.cfgpath .. "  // " .. v.meta.prompt
		sortedList[#sortedList + 1] = v
	end
	local object = selectList("Select Platform", prompts, sortedList)
	return object
end

---------------------------------------------------------------------
-- Main
---------------------------------------------------------------------
function main(forceProject)

	local promptList, confList, confSelected

	printf("\n")
	printf("==============================================\n")
	printf("== Configuration Setup\n")
	printf("==============================================\n")

	config.HOST = "linux-x86"
	
	-------------------------------------
	-- Project
	-------------------------------------
--[[
	local projects = findProjects("project")
	local project = selectProject(projects)
	assert(project ~= nil)
	config._project = project
	config.PROJECT = project.name
	config.PROJECT_DIR = project.path
	IncludeConfig(project.path .. "/project.cfg")
--]]
	local projects = findProjectConfigs("project")
	local project = selectProjectConfig(projects)
	assert(project ~= nil)
	config._project = project
	config.PROJECT = project.name
	config.PROJECT_DIR = project.path
	IncludeConfig(project.cfgpath)

	-------------------------------------
	-- Platform
	-------------------------------------
--[[	
	local platforms = findPlatforms("platform")
	local platform
	if config.PLATFORM == nil then
		platform = selectPlatform(platforms)
		config.PLATFORM = platform.name
	else
		platform = platforms[config.PLATFORM]
		assert(platform ~= nil)
	end
	config._platform = platform
	config.PLATFORM_DIR = platform.path
	IncludeConfig(platform.path .. "/platform.cfg")]]--
	local platforms = findPlatformConfigs("platform")
	local platform = selectPlatformConfig(platforms)
	assert(platform ~= nil)
	config._platform = platform
	config.PLATFORM = platform.name
	config.PLATFORM_DIR = platform.path
	IncludeConfig(platform.cfgpath)

	if config.TARGET == nil then
		config.TARGET = "linux-arm"
	end
	if config.SIMULATOR == nil then
		config.SIMULATOR = false
	end

	-------------------------------------
	-- Product
	-------------------------------------
	local product = {
		name = project.name .. "__" .. platform.name,
		project = project,
		platform = platform
	}
	product.path = "out/" .. product.name
	config.PRODUCT = product.name
	config._product = product.name

	-------------------------------------
	-- Collect configs
	-------------------------------------
	IncludeConfig("sdk/mkconfig")
	IncludeConfig("project/mkconfig")
	IncludeConfig("platform/mkconfig")

	-------------------------------------
	-- Create config files.
	-------------------------------------
	dumpConfig(config)

--	saveToFile("product.mak", string.format(
--		"PRODUCT := %s\nPROJECT := %s\nPLATFORM := %s\n\n" ..
--		"PRODUCT_DIR := %s\nPROJECT_DIR := %s\nPLATFORM_DIR := %s\n",
--		product.name, project.name, platform.name,
--		product.path, project.path, platform.path))
	saveToFile("product_config.mak", string.format("PRODUCT := %s\n", product.name))

	lfs.mkdir("out")
	lfs.mkdir(product.path)
	lfs.mkdir(product.path .. "/config")
	saveConfig(product.path .. "/config/sysconfig.mak", config)
	printf("Save configuration to %s\n", product.path .. "/config/sysconfig.mak")
	saveToFile(product.path .. "/config/sysconfig.h", configToHeaderFormat(config))
	saveToFile(product.path .. "/config/sysconfig.lua", configToLuaFormat(config))

	saveToFile(product.path .. "/Makefile", string.format(
[[### DO NOT EDIT THIS FILE ###
TOPDIR := ../../
PRODUCT := %s
include $(TOPDIR)sdk/build/core/product.mak
### DO NOT EDIT THIS FILE ###
]], product.name))

end


function getPlatformPath(platformSelected)
	IncludeConfig("platform/mkconfig")

	-- trim the space
	platformSelected = string.match(platformSelected, "([_%w]+)")

	local promptList, confList, pathList = parseConfigList(CONFIG.Platform)
	local configIndex = indexOfTable(confList, platformSelected)
	io.stdout:write(pathList[configIndex])
end


io.output():setvbuf("no")

if arg[1] == "--non-interactive" and arg[2] then
	-- Override select*() functions to assert unspecified configs.
	selectList = function(title, list, valueList) error(string.format('"%s" is unspecified', title)) end
	selectBool = function(title, message) error(string.format('"%s" is unspecified', title)) end
	main(arg[2])
elseif arg[1] == "--get-platform-path" then
	getPlatformPath(arg[2])
else
	main()
end

