-- Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz 
-- or its licensors, as applicable.
-- 
-- You may not use this file except under the terms of the accompanying license.
-- 
-- Licensed under the Apache License, Version 2.0 (the "License"); you
-- may not use this file except in compliance with the License. You may
-- obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
-- 
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
-- 
-- Project: ocroscript
-- File: util.lua
-- Purpose: several function that didn't fit anywhere else
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


require 'lib.hocr'
require 'lib.xml'

util = {}

function util.get_bpnet_path(opt)
    local ocrodata = '.:../data/models:/usr/local/share/ocropus'
    ocrodata = os.getenv('OCRODATA') or ocrodata
    local search_path = path.join(ocrodata, 'models')
    search_path = opt.bpnetpath or os.getenv 'bpnetpath' or search_path

    local raw_bpnet_path = opt.bpnet or os.getenv 'bpnet'
    if raw_bpnet_path == true then
        raw_bpnet_path = 'neural-net-file.nn'
    end

    local bpnet_path
    if raw_bpnet_path then
        bpnet_path = path.search(search_path, raw_bpnet_path)
    end
    if raw_bpnet_path and not bpnet_path then
        print "Unable to find bpnet!"
        print("Bpnet search path:", bpnet_searchpath)
        print("Bpnet file to search:", raw_bpnet_path)
        os.exit(1)
    end
    return bpnet_path
end

--- Returns the hashtable of options and the array of remaining arguments.
--- Typical usage: opt,arg = getopt(arg)
--- @param arg The list of arguments, most likely "arg" provided by ocroscript
--- @return opt, arg where opt is the hashtable of options,
---         and arg is the list of non-option arguments.
function util.getopt(args)
    local opts = {}
    local remaining_args = {}
    remaining_args[0] = args[0]
    i = 1
    -- I haven't used a for loop because at some point
    -- we can support "-key value" instead of "-key=value".
    while i <= #args do
        if args[i]:match('^-') then
            local option = args[i]
            option = option:gsub('^%-*','') -- trim leading `-' or `--'
            local key, val = option:match('^(.-)=(.*)$')
            if key and val then
                opts[key] = val
            else
                opts[option] = true
            end
        else
            table.insert(remaining_args, args[i])
        end
        i = i + 1
    end
    return opts, remaining_args
end

--- Read an image and abort if it's not there.
--- Any OCRopus-supported format should work;
--- that doesn't include TIFF at the moment (a2).
--- @param path The path to the image file.
--- @return The image (bytearray).
function util.read_image_gray_checked(path)
    local image = bytearray()
    if not pcall(function() iulib.read_image_gray(image, path) end) then
        print(("Unable to load gray image `%s'"):format(path))
        os.exit(1)
    end
    return image
end

function util.read_image_color_checked(path)
    local image = intarray()
    if not pcall(function() iulib.read_image_packed(image, path) end) then
        print(("Unable to load rgb image `%s'"):format(path))
        os.exit(1)
    end
    return image
end

function util.secure_open(path, mode)
    file = io.open(path,mode)
    if not file then
	    print(("Unable to open `%s' in mode `%s'"):format(path,mode))
	    os.exit(1)
    end
    return file
end
