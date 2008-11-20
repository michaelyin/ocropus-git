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
-- File: headings.lua
-- Purpose: detecting headings
-- Responsible: mezhirov (the original code is by Ambrish Dantrey)
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


require 'lib.util'
require 'lib.hocr'

local function confirm_header(result, skips, heights, j)
    local afterskip_coef = .7

    -- check if we're a 1-line header
    if #result > 1 and j + 1 <= #result
        and skips[j] > 1.5 * heights[j] 
        and skips[j + 1] > afterskip_coef * heights[j]
    then
        return true
    end
                  
    -- check if we're the first line of a 2-line header
    if j + 2 <= #result
        and skips[j] > 1.5 * heights[j]
        and skips[j + 2] > afterskip_coef * heights[j + 1]
    then
        return true
    end

    -- check if we're the second line of a 2-line header
    return j > 0
        and j < #result
        and result[j-1]
        and skips[j + 1] > afterskip_coef * heights[j]
end

local function preinit_header_flags_by_medians(line_medians)
    if #line_medians <= 1 then
        return {}
    end
    local result = {}
    for i = 1, #line_medians do
        result[i] =(i < #line_medians 
                    and i > 1 
                    and line_medians[i] >= 1.4 * line_medians[i + 1]  
                    and line_medians[i] >= 1.1 * line_medians[i - 1]
                    )

                    or (i == 1
                    and line_medians[i] >= 1.5 * line_medians[i + 1])

                    or (i + 2 <= #line_medians
                    and line_medians[i] >= 1.5 * line_medians[i + 2])
        
                    or (i + 3 <= #line_medians
                    and line_medians[i] >= 1.6 * line_medians[i + 3])

                    or (i + 4 <= #line_medians
                    and line_medians[i] >= 1.7 * line_medians[i + 4])           
    end
    return result
end

--- Detect headings.
--- @param bboxes
--- @param binary_page_image A binary image of the page.
--- @return A Lua table (numbered from 1, as the regions) of booleans.
local function detect_headings_internal(bboxes, binary_page_image)
    local line_medians = {}
    hist = floatarray(100)
    page_hist = floatarray(100)
    narray.fill(hist, 0)
    narray.fill(page_hist, 0)
    for i = 1, #bboxes do
        mask = bytearray()
        iulib.extract_subimage(mask, binary_page_image, bboxes[i].x0, bboxes[i].y0, 
                                                  bboxes[i].x1, bboxes[i].y1)
        make_background_white(mask)
        runlength_histogram(hist, mask)
        narray.add(page_hist, hist)
        line_medians[i] = find_median_in_histogram(hist)
    end

    local median = find_median_in_histogram(page_hist)
    local flags = preinit_header_flags_by_medians(line_medians)

    heights = {}
    for i = 1, #bboxes do
        heights[i] = bboxes[i]:height()
    end

    skips = {100000}
    for i = 2, #bboxes do
        skips[i] = bboxes[i-1].y0 - bboxes[i].y1
    end

    local result = {}
    -- throw out some headings
    for i = 1, #flags do
        if flags[i] and (line_medians[i] <= median + 1 
                         or confirm_header(flags, skips, heights, i)) then
            table.insert(result, i)
        end
    end
    return result
end

-- Here we consider all lines inside the page, including non-direct children.
function detect_headings(page_DOM, page_image)
    local indices = {}
    local bboxes = {}
    hocr.find_lines_recursive(indices, bboxes, page_DOM, page_image)
    local headers = detect_headings_internal(bboxes, page_image)
    -- change line numbers to node indices (in case we have non-line nodes)
    for i = 1, #headers do
        local k = indices[headers[i]]
        page_DOM[k] = {tag = 'h3', page_DOM[k]}
    end
    return page_DOM
end
