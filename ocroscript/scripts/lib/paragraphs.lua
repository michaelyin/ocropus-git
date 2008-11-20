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
-- File: paragraphs.lua
-- Purpose: detecting paragraphs
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

require 'lib.util'
require 'lib.hocr'

--- Detect paragraphs in the text
--- and return a Lua array with indices
local function detect_paragraph_starters(bboxes)
    local result = {}
    for i = 1, #bboxes - 1 do
        local rect = bboxes[i]
        local next = bboxes[i + 1]
        -- A line is a paragraph starter if, compared to the next line:
        -- it's above
        -- its right edge is more or less the same
        -- its left edge is shifted to the right
        if next.y0 < rect.y0
           and math.abs(next.x1 - rect.x1) < 0.5 * rect:height()
           and rect.x0 - next.x0 > 0.5 * rect:height()
        then
            table.insert(result, i)
        end
    end
    return result
end

local function fold(new_node, old_node, start, finish --[[incl.]])
    if start >= finish then return end
    tag = {tag = 'p', class = 'ocr_par'}
    for i = start, finish do
        table.insert(tag, old_node[i])
    end
    table.insert(new_node, tag)
end

-- Here we only consider immediate children of the page node.
function detect_paragraphs(page_DOM, page_image)
    local indices, bboxes = hocr.find_lines(page_DOM, page_image)
    local starters = detect_paragraph_starters(bboxes)
    if #starters == 0 then
        return page_DOM
    end
    -- change line numbers to node indices (in case we have non-line nodes)
    for i = 1, #starters do
        starters[i] = indices[starters[i]]
    end
    local new_DOM = hocr.copy_DOM_node(page_DOM)
    fold(new_DOM, page_DOM, 1, starters[1] - 1)
    for i = 1, #starters - 1 do
        fold(new_DOM, page_DOM, starters[i], starters[i+1] - 1)
    end
    fold(new_DOM, page_DOM, starters[#starters], #page_DOM)
    return new_DOM
end
