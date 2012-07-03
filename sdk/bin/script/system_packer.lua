require "lfs"
require "struct"
require "bit"
require "crc"
require "encdec"

local function print_usage()
	print(string.format("Usage:\n\t%s <product_dir> <firmware_package.lua> <package_name> [<cipherkey>]", arg[0]))
end

local function generate_package(srcpath, package, outname, cipherkey)
	-- Check
	local version = package.version
	if not version or not version.major or not version.minor then
		error("No version found.")
	end

	if not package.description then
		package.description = {encrypted = cipherkey and true or false}
	end
	if not package.sections or #package.sections < 1 then
		error("No section found.")
	end
	if #package.sections > 16 then
		error("Too many sections found (" .. #package.sections .. ").")
	end

	for i, section in ipairs(package.sections) do
		if not section.name or #section.name < 1 then
			error("Undefined section name for section number " .. i)
		end
		if #section.name > 13 then
			error("Section name should less than 13 bytes, section number " .. i)
		end

		if not section.partition_name or #section.partition_name < 1 then
			error("Undefined section partition_name for section number " .. i)
		end
		if #section.partition_name > 13 then
			error("Section partition_name should less than 13 bytes, section number " .. i)
		end

		if section.data then
			local size, err = lfs.attributes(srcpath .. section.data, "size")
			if not size then
				error(err)
			end
			if section.partition_size and section.partition_size < size then
				error("Partition size should be greater then data size for section number " .. i)
			end
		elseif not section.partition_size then
			error("Neither data nor partition_size is defined for the section " .. section.name)
		end
	end

	-- sections
	local body = ""

	local section_offset = 1024
	for i, section in ipairs(package.sections) do
		if section.data then
			local datasize = lfs.attributes(srcpath .. section.data, "size")
			local d = section.data and io.open(srcpath .. section.data, "rb")
			local data = d and d:read(datasize)
			if d then d:close() end
			body = body .. data

			local checksum = crc.calc(data)
			section.crc = checksum
			section.offset = section_offset
			section_offset = section_offset + datasize
		end
	end

	-- header
	local header1 = ""
	local header2 = ""

	-- version, signature, image description and section version
	local sg, sp, sm = string.byte("GPM", 1, 3)
	local data = struct.pack("<i4i4i4",
							 bit.lshift(version.major, 16) + version.minor,
							 bit.lshift(sg, 24) + bit.lshift(sp, 16) + bit.lshift(sm, 8) + sp,
							 package.description.encrypted and 1 or 0)
	header1 = header1 .. data

	-- sections in header
	local data = struct.pack("<i4", package.sections.version or 0)
	header2 = header2 .. data
	for _, section in ipairs(package.sections) do
		
		local datasize = section.data and lfs.attributes(srcpath .. section.data, "size") or 0
		local data = struct.pack("<i2c13c13i4i4i4i1i1i2i4",
								 section.version or 0,
								 section.name .. string.rep("\000", 13),
								 section.partition_name .. string.rep("\000", 13),
								 section.offset or 0,
								 datasize,
								 section.partition_size or 0,
								 section.partition_type or 0,
								 section.partition_flag or 0,
								 0,
								 section.crc or 0)

		header2 = header2 .. data
	end

	-- header paddings
	local data = struct.pack("<" .. string.rep("x", 1024 - 4 - #header1 - #header2))
	header2 = header2 .. data

	-- header checksum
	local checksum = crc.calc(header1 .. header2)
	local data = struct.pack("<i4", checksum)
	header2 = header2 .. data

	-- whole checksum
	local checksum = crc.calc(header1 .. header2 .. body)
	local checksum = struct.pack("<i4", checksum)

	-- Write to file.
	local f = io.open(outname, "wb")
	if not f then
		error(string.format("Failed to write file %s", outname))
	end

	local image = header2 .. body .. checksum
	if cipherkey then
		image = encdec.encode(image, cipherkey)
	end

	f:write(header1 .. image)
	f:close()

	return true
end

local function main()
	local srcpath = arg[1]
	local filename = arg[2]
	local outname = arg[3]
	local cipherkey = arg[4]
	if not filename or not outname then
		print_usage()
		return
	end

	local package = assert(loadfile(filename))()
	local res, err = generate_package(srcpath, package, outname, cipherkey)
	if not res then
		error(err)
	end
end

main()
