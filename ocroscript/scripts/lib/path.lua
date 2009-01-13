-- Copyright 2006-2007 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz 
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
-- File: path.lua
-- Purpose: path manipulations similar to Python's os.path
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


path = {}

--- Prepend backslashes before every occurence of the given pattern,
--- and backslashes themselves are transformed into double backslashes.
local function escape(s, pattern)
    return (s:gsub('\\', '\\\\'):gsub(pattern, function(x) return '\\'..x end))
end

--- Drop backslash escapes.
local function unescape(s)
    return (s:gsub('\\+', function(x) return string.rep('\\', div(#x, 2)) end))
end

local function parameter(path)
    unescaped = '"'..escape(path, '"')..'"'
    return unescaped
end

local function test(path, code)
    assert(#code == 1)
    return os.execute('test -'..code..' '..parameter(path)) == 0;
end

--- Checks if the given path is absolute.
function path.isabs(path)
    return path:match('^/') or path:match('^%a:\\')
end

--- Checks if the given file is a directory.
function path.isdir(path)
    return test(path, 'd')
end

--- Checks if the given file exists.
function path.exists(path)
    return test(path, 'e')
end

--- Join one or more path components.
function path.join(...)
    local list = {...}
    assert(#list >= 1)
    local result = list[#list]
    for i = #list - 1, 1, -1 do
        local s = list[i]
        assert(s)
        if s ~= '.' then
            if s:match('/$') then
                result = s..result
            else
                result = s..'/'..result
            end
            if path.isabs(s) then
                break
            end
        end
    end
    return result
end


function path.dirname(p)
    return p:match("^.*[/\\]") or '.'
end

function path.basename(p)
    return p:gsub("^.*[/\\]","")
end


--- Search for a file in the given path.
--- @param p            The path to search in, might be a table or a string
--                      with path elements separated by ':' or ';'
--- @param file_name    The name to search
--- @return Full path of the file if found, nil if not found.
function path.search(p, file_name)
    if type(p) == 'string' then
        local list = {}
        for i in p:gmatch('[^:;]+') do
            table.insert(list, i)
        end
        p = list
    end
    for _, dir in pairs(p) do
        local full_path = path.join(dir, file_name)
        if path.exists(full_path) then
            return full_path
        end
    end
end
