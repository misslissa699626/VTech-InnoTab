require "common"
lfs = require "lfs"

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

function saveToFile(filename, str)
	local f = assert(io.open(filename, "wb"));
	f:write(str)
	f:close();
end

---------------------------------------------------------------------
-- Main
---------------------------------------------------------------------
function findProducts(path)
	local list = {}
	for dname in lfs.dir(path) do
		if dname:sub(1,1) ~= "." then
			local dirname = path .. "/" .. dname
			local attr = lfs.attributes(dirname)
			if attr.mode == "directory" then
				local configFile = dirname .. "/" .. "config/sysconfig.mak"
				local attr = lfs.attributes(configFile)
				if attr and attr.mode == "file" then
					list[dname] = {
						name = dname,
					}
				end
			end
		end
	end
	return list
end

function selectProducts(list, prompt)
	local prompts = {}
	local sortedList = {}
	for k, v in pairsBySortedKey(list) do
		prompts[#prompts + 1] = v.name
		sortedList[#sortedList + 1] = v
	end
	local object = selectList(prompt, prompts, sortedList)
	return object
end
	

function main()
	local products = findProducts("out")
	local product = selectProducts(products, "Select Product:")
	saveToFile("product_config.mak", string.format("PRODUCT := %s\n", product.name))
	print("Switch to product : " .. product.name .. "\n")
end


main()

