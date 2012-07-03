
function checkKernelConfig(kernelPath, arch)
	local flag = false
	local pattern = "CONFIG_ARCH_" .. arch .. "=y"
	for line in io.lines(kernelPath .. "/.config") do
		if string.find(line, pattern) then
			flag = true
			break;
		end
	end
	
	return flag
end

function grepKernelConfigArch(kernelPath)
	local arch
	local configFile

	local file = io.open(kernelPath .. "/.config", "r")
	if not file then
		return nil
	end
	
	for line in file:lines() do
		arch = string.match(line, "CONFIG_ARCH_([^_]*)=y")
		if arch then
			break;
		end
	end
	for line in file:lines() do
		local defArch, defFile = string.match(line, "CONFIG_DEFCONFIG_(.*)=\"(.*)\"")
		if defArch == arch then
			--print("DEFCONFIG", defFile)
			configFile = defFile
			break;
		end
	end
	file:close()
	
	return arch, configFile
end

function configKernel(kernelPath, arch, configFile)
	if not configFile then
		if arch == "SPMP8050" then
			configFile = "spmp8050_defconfig"
		elseif arch == "GPL32900" then
			configFile = "gpl32900_defconfig"
		elseif arch == "GPL64000" then
			configFile = "gpl64000_defconfig"
		else
			print("Unknown arch")
			return false
		end
	end
	
	os.execute(string.format("cd %s; make %s", kernelPath, configFile))
	return true
end

function main(kernelPath, arch, configFile)
	local kernelArch, defconfig = grepKernelConfigArch(kernelPath)
	if kernelArch == nil then
		print("Kernel arch config is nil, reconfig kernel to " .. arch)
		configKernel(kernelPath, arch, configFile)
		print("Reconfig kernel done.\n")
	elseif kernelArch ~= arch then
		print("Kernel arch config : " .. kernelArch)
		print("SYSCONFIG_ARCH : " .. arch)
		print("Kernel arch not match, reconfig kernel to " .. arch)
		configKernel(kernelPath, arch, configFile)
		print("Reconfig kernel done.\n")
	elseif configFile and defconfig ~= configFile then
		print("Kernel config file : " .. (defconfig or "unknown"))
		print("Wish config file : " .. configFile)
		print("Kernel config file not match, reconfig kernel to " .. configFile)
		configKernel(kernelPath, arch, configFile)
		print("Reconfig kernel done.\n")
	else
		--print("Kernel arch config : " .. kernelArch)
		--print("SYSCONFIG_ARCH : " .. arch)
	end
end

main(...)

