--require "w32"

---------------------------------------------------------------------
-- Common API
---------------------------------------------------------------------

function printf(fmt, ...)
	return io.write(string.format(fmt, ...));
end


function sprintf(fmt, ...)
	return string.format(fmt, ...);
end


function fprintf(f, fmt, ...)
	return f:write(string.format(fmt, ...));
end


function convertByteToViewableChar(b)
	if (b <= 32 or b >= 127) then
		return ".";
	else
		return string.char(b);
	end
end


function BinToHexDump(s, indent)
	indent = indent or "";
	local h = "";
	local eh = "";
	for i = 1, #s do
		if ((i % 16) == 1) then
			h = h .. indent;
		end

		h = h .. string.format("%02X ", s:byte(i));
		eh = eh .. convertByteToViewableChar(s:byte(i));
		if ((i % 16) == 0) then
			h = h .. "   " .. eh .. "\n";
			eh = "";
		elseif (i == #s) then
			h = h .. "   " .. string.rep("   ", 16 - (i % 16)) .. eh .. string.rep(" ", 16 - (i % 16)) .. "\n";
			eh = "";
		end
	end
	return h;
end


function BinToHex(bin)
	local hex = "";
	for i = 1, #bin do
		hex = hex .. string.format("%02X", bin:byte(i));
	end
	return hex;
end

function HexToBin(hex)
    -- hex to bin
    local bin = ""
    for i = 1, #hex, 2 do
	    val = tonumber(hex:sub(i, i+1), 16);
	    bin = bin .. string.char(val);
	end
	return bin;
end

function pairsBySortedKey(t, f)
	local a = {};
	for k in pairs(t) do a[#a + 1] = k end
	table.sort(a, f);
	local i = 0;
	return function ()
		i = i + 1;
		return a[i], t[a[i]];
	end
end



---------------------------------------------------------------------
-- StringBuffer
---------------------------------------------------------------------

StringBuffer = {}

function StringBuffer:New(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    o.buf = ""
    return o
end

function StringBuffer:WriteInt32(v)
    local hex = string.format("%08X", v);
    local bin = ""
    for i = #hex, 2, -2 do
	    local byte = tonumber(hex:sub(i-1, i), 16);
	    bin = bin .. string.char(byte);
	end
    self.buf = self.buf .. bin;
end

function StringBuffer:WriteInt16(v)
    local hex = string.format("%04X", v);
    local bin = ""
    for i = #hex, 2, -2 do
	    local byte = tonumber(hex:sub(i-1, i), 16);
	    bin = bin .. string.char(byte);
	end
    self.buf = self.buf .. bin;
end

function StringBuffer:WriteInt8(v)
    local hex = string.format("%02X", v);
    local bin = ""
    for i = #hex, 2, -2 do
	    local byte = tonumber(hex:sub(i-1, i), 16);
	    bin = bin .. string.char(byte);
	end
    self.buf = self.buf .. bin;
end

function StringBuffer:WriteInt(v)
    self:WriteInt32(v);
end

function StringBuffer:WriteString(str)
    self.buf = self.buf .. str;
end

function StringBuffer:WriteHexString(str)
    self.buf = self.buf .. HexToBin(str);
end

function StringBuffer:WriteFormat(fmt, ...)
    self.buf = self.buf .. string.format(fmt, ...);
end

function StringBuffer:ToString()
    return self.buf;
end

function StringBuffer:printf(fmt, ...)
    self.buf = self.buf .. string.format(fmt, ...);
end
