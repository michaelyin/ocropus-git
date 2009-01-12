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
-- File: rec-bpnet-isolated.lua
-- Purpose: recognizing isolated characters with a backpropagation net
-- Responsible: rangoni
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

require 'lib.util'
require 'lib.datasets'

opt,arg = util.getopt(arg)

if #arg ~= 2 then
    print ('Usage: ocroscript rec-bpnet-isolated <bpnet-file> <dataset>')
    print '     <bpnet-file> - a file containing a trained bpnet'
    print '     <dataset> - a list of "image_path text_path" lines'
    print 'Isolated testing: every line should contain one character only'
    print 'Options:'
    print '     --degrade=...       - the number of passes with degradation'
    print '                           (set to 0 to make one pass on pure data)'
    os.exit(1)
end

function rec_chars(database, cc, degradations)
	local total = 0
	local errors = 0
	local s = nustring()
	for key, i in pairs(database) do
	    local image = bytearray:new()
		narray.copy(image, i.image)
		for d = 1, degradations do                 --just degarde for fun
			degrade(image)
		end
	    cc:setImage(image)
		cc:best(s)
		total = total + 1
		if (s:utf8() ~= i.text:utf8()) then
			print(string.format("out:%s    target:%s", s:utf8(), i.text:utf8()))
			errors = errors + 1
		end
	end
	print(string.format("error: %d/%d  %g%%",errors,total,errors*100./total))
end

local mlp_path = arg[1]
local dataset_path = arg[2]

local degradations = opt.degrade or 50

-- load the data
local test_data = {}
for image, text in datasets.dataset_entries(dataset_path) do
	if (#text ~= 1) then
		print "you must provide only one character in the transcription"
    	assert(#text == 1) -- transcript is actually an array of nustrings
	end
    text = text[1] -- but there should be only one item in the array
    table.insert(test_data, {image = image, text = text})
end

--create a bpnet classifier for isolated characters
local cc = make_BpnetCharacterClassifier() 

--just load a existing file containing a trained bpnet		
cc:load(mlp_path)

--and test on database
rec_chars(test_data, cc, degradations)

--end
