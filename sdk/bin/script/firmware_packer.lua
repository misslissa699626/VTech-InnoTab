require "struct"

local function print_usage()
	print(string.format("Usage:\n\t%s kernel=<kernel.img> system=<system.img> out=<output.bin>", arg[0]))
end

local function packer_create(outname)
	-- Open output file.
	local f = io.open(outname, "wb")
	if not f then
		error(string.format("Failed to write file %s", outname))
		return nil
	end

	local packer = {
		outname = outname,
		f = f,
		sections = {},
		offset = 0,
	}

	return packer
end

local function packer_addSection(packer, sectName, sectFile)
	local infile = io.open(sectFile, "rb")
	local data = infile:read("*a")
	local size = infile:seek("end", 0)
	infile:close()

	section = {
		name = sectName,
		file = sectFile,
		data = data,
		len = size,
	}
	packer.sections[#packer.sections + 1] = section

	return section
end

local function packer_generate(packer)
	local n = 0
	local f = packer.f
	local offset = (#packer.sections + 1) * 24
	
	-- generate headers
	for i, v in ipairs(packer.sections) do
		v.offset = offset
		offset = offset + v.len
		local header = struct.pack("<c16i4i4", v.name .. string.rep("\000", 16), v.offset, v.len)
		f:write(header)
		n = n + #header
	end
	local header = struct.pack("<c16i4i4", string.rep("\000", 16), 0, 0)
	f:write(header)
	n = n + #header

	-- write data
	for i, v in ipairs(packer.sections) do
		f:write(v.data)
		n = n + #v.data
	end
	
	-- align 4 bytes
	if (n % 4) ~= 0 then
		local len = 4 - (n % 4)
		f:write(string.rep("\000", len))
		n = n + len
	end
end

local function packer_destroy(packer)
	packer.f:close()
end

local function main()
	local argList = {}

	for i = 1, #arg do
		local s1, s2 = string.match(arg[i], "([^=]+)=(.+)")
		argList[s1] = s2
	end

	if not argList["kernel"] or not argList["system"] or not argList["out"] then
		print_usage()
		return
	end
	local out = argList["out"]
	argList["out"] = nil

	local packer, err = packer_create(out)
	packer_addSection(packer, "kernel", argList["kernel"])
	packer_addSection(packer, "system", argList["system"])
	packer_generate(packer)
	packer_destroy(packer)
end

main()
