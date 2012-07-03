#!/system/bin/lua
package.path = "./?.lua;/system/lua/?.lua"
package.cpath = "./?.so;/system/lua/clib/?.so"

local lfs = require "lfs"
local struct = require "struct"

--print("Hello Lua!")
--print(os.getenv("LD_LIBRARY_PATH"))

