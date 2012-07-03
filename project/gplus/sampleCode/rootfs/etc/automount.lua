#!/system/bin/lua
package.path = "./?.lua;/system/lua/?.lua"
package.cpath = "./?.so;/system/lua/clib/?.so"

local lfs = require "lfs"

local event = {
	dev = os.getenv("MDEV") or "",
	action = os.getenv("ACTION") or "",
	path = os.getenv("DEVPATH") or "",
	subsystem = os.getenv("SUBSYSTEM") or "",
	mntdir = os.getenv("MNT_DIR") or "",
}

-- Logging
local f = io.open("/hotplug.log", "a")
local function log(...)
	f:write(string.format(...))
end

log("action: %s\nmdev: %s\ndevpath: %s\nsubsystem: %s\nmntdir: %s\n",
	event.action,
	event.dev,
	event.path,
	event.subsystem,
	event.mntdir)

-- Check if USB or SD
local mountPoint
local isUsb = string.match(event.dev, "^sd[a-d][1]?$")
if isUsb then
	log("Usb disk : %s\n", event.dev)
	mountPoint = "/media/" .. event.dev
end

local isSdcard = string.match(event.dev, "^sdcard[a-d][1]?$")
if isSdcard then
	log("SD disk : %s\n", event.dev)
	mountPoint = "/media/" .. event.dev
end

-- Mount/Unmount
if not mountPoint then
	-- skip
elseif event.action == "add" then
	os.execute(string.format("/bin/mkdir -p %s", mountPoint))
	local ret = os.execute(string.format("/bin/mount /dev/%s %s", event.dev, mountPoint))
	log("Mount result: %d\n", ret)
	if ret ~= 0 then -- mount failed
		os.execute(string.format("/bin/rmdir %s", mountPoint))
	end
elseif event.action == "remove" then
	local attr = lfs.attributes(mountPoint)
	if attr and attr.mode == "directory" then
		local ret = os.execute(string.format("/bin/umount -lf %s", mountPoint))
		log("Umount result: %d\n", ret)
		os.execute(string.format("/bin/rmdir %s", mountPoint))
	end
end

log("\n")
f:close()
