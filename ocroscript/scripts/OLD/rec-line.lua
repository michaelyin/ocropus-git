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
-- File: rec-line.lua
-- Purpose: use a trained bpnet to recognize an isolated line
-- Responsible: rangoni
-- Reviewer:
-- Primary Repository:
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

require 'lib.util'
require 'lib.path'
import_all(ocr)
opt,arg = util.getopt(arg)

if #arg ~= 1 then
    print "Usage: ocroscript rec-line <line>"
    print "Options:"
    print "       --bpnet=<path>       Load a trained neural net, if not set"
    print "                            the script uses ../data/models/neural-net-file.nn"
    os.exit(1)
end

bpnet_path = util.get_bpnet_path(opt)   -- look for a bpnet path
local c = make_BpnetClassifier()        -- create mlp
local cc = make_AdaptClassifier(c)      -- create character classifier
cc:load(bpnet_path)                     -- load trained mlp

local seg = make_CurvedCutSegmenter()   -- create segmenter
local ocr = make_NewGroupingLineOCR(cc, seg, true) --if the bpnet contains line info, set to true

local line = util.read_image_gray_checked(arg[1])   -- load the image

local fst = make_StandardFst()          -- create fst
ocr:recognizeLine(fst, line)            -- recognize line

local s = nustring()                    -- create empty string
fst:bestpath(s)                         -- get best path of the fst and put the result in the empty string

print(s:utf8())                         -- print the transcription
