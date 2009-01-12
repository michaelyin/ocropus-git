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
-- File: train-bpnet-lines.lua
-- Purpose: training a backpropagation net on segmented lines
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

import_all(ocr)
require 'lib.util'
require 'lib.datasets'

opt,arg = util.getopt(arg)

if #arg ~= 2 then
    print ('Usage: ocroscript '..arg[0]..' [options] <dataset> <output>')
    print '     <dataset> - a list of "image_path text_path segmentation" lines'
    print 'Isolated training: every line should contain one character only'
    print 'Options:'
    print '     --dataset-mode=...  - set to "onlyseg" if you have no grays'
    print '     --maxlines=...      - stop after the specified amount of lines'
    print '     --nhidden=...       - the number of hidden units'
    print '     --epochs=...        - the number of epochs to train'
    print '     --learningrate=...  - weights change coefficient'
    print '     --testportion=...   - how much data to leave for validation'
    print '     --normalize=...     - normalize means and dispersions of input'
    print '     --shuffle=...       - randomize the order of data'
    os.exit(1)
end

local dataset_path = arg[1]
local result_path = arg[2]

-- make the IRecognizeLine to train
local c = make_BpnetClassifierDumpIntoFile(arg[2])
c:param("nhidden", opt.nhidden or 200)
c:param("epochs", opt.epochs or 50)
c:param("learningrate", opt.learningrate or .2)
c:param("testportion", opt.testportion or .1)
c:param("normalize", opt.normalize or 1)
c:param("shuffle", opt.shuffle or 1)

function concatenate_transcript(transcript)
    -- this is an insane way of concatenating!
    local s = ""
    for i = 1, #transcript do
        s = s..transcript[i]:utf8()
    end
    return nustring(s)
end

-- load & train
local log = Logger("train")
local cc = make_AdaptClassifier(c, true)
local segmenter = make_CurvedCutSegmenter()
line_ocr = make_NewGroupingLineOCR(cc, segmenter, true)
line_ocr:startTraining()
i = 0

for image, text, seg in datasets.dataset_entries(dataset_path) do
    log:log("transcript", concatenate_transcript(text):utf8())
    log:recolor("segmentation", seg)
    line_ocr:addTrainingLine(seg, image, concatenate_transcript(text))
    i = i + 1
    print(i .. " lines added to the recognizer")
    if opt.maxlines and i >= tonumber(opt.maxlines) then
        break
    end
end

print "Training..."
line_ocr:finishTraining()
line_ocr:save(result_path)
print "End"
