-- /**************************************************************************
-- **                                                                        *
-- **         Copyright (c) 2008 by Sunplus mMedia Inc.                      *
-- **                                                                        *
-- **  This software is copyrighted by and is the property of Sunplus        *
-- **  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
-- **  This software may only be used in accordance with the                 *
-- **  corresponding license agreement. Any unauthorized use, duplication,   *
-- **  distribution, or disclosure of this software is expressly forbidden.  *
-- **                                                                        *
-- **  This Copyright notice MUST not be removed or modified without prior   *
-- **  written consent of Sunplus mMedia Inc.                                *
-- **                                                                        *
-- **  Sunplus mMedia Inc. reserves the right to modify this software        *
-- **  without notice.                                                       *
-- **                                                                        *
-- **  Sunplus mMedia Inc.                                                   *
-- **  19, Innovation First Road, Science-Based Industrial Park,             *
-- **  Hsin-Chu, Taiwan, R.O.C.                                              *
-- **                                                                        *
-- **                                                                        *
-- ***************************************************************************/

-- Pad the firmware to the required size.
-- Example: lua padding.lua spmp2800.bin spmp2800_padded.bin 2097152

-- Input filename
local input_filename = arg[1] or error("Input filename unknown.")
-- Output filename
local output_filename = arg[2] or error("Output filename unknown.")
-- Desired output size, in bytes
local target_size = arg[3] or error("Target size unknown.")

local MAGIC = "SPMF"

local infile = io.open(input_filename, "rb")
local data = infile:read("*a")
local size = infile:seek("end", 0)
infile:close()

local outfile = io.open(output_filename, "wb")
outfile:write(data)
outfile:seek("cur", -#MAGIC)
outfile:write(string.rep("\0", target_size - size))
outfile:write(MAGIC)
outfile:close()
