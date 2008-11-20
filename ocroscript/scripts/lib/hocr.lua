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
-- File: hocr.lua
-- Purpose: hOCR output
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

hocr = {}

function hocr.parse_rectangle(s, h)
    local x0, y0, x1, y1 = s:match('^%s*(%d*)%s*(%d*)%s*(%d*)%s*(%d*)%s*$')
    assert(x0 and y0 and x1 and y1, "rectangle parsing error: "..s)
    return rectangle(tonumber(x0), h-1-y1, tonumber(x1), h-1-y0)
end

--- Represent a rectangle as a string (converting to top-down coordinates).
function hocr.bbox_to_string(page, bbox)
    if page and bbox then
        assert(bbox.x0 >= 0 and bbox.x1 >= 0, "we only store in-image boxes")
        local h
        if page.height then
            h = page:height()
        else 
            h = page:dim(1)
        end
        return string.format('%d %d %d %d',
                              bbox.x0,      -- left
                              h - bbox.y1,  -- top
                              bbox.x1,      -- right
                              h - bbox.y0)  -- bottom
    else
        return nil
    end
end


function hocr.bboxes_to_string(page, bboxes)
    if #bboxes == 0 then
        return nil
    end
    local s = hocr.bbox_to_string(page, bboxes[1])
    for i = 2, #bboxes do
        s = s..', '..hocr.bbox_to_string(page, bboxes[i])
    end
    return s
end

function hocr.get_meta_tag(header, meta, default)
    local t = header[meta]
    if not t then
        t = default
    end
    return {tag = 'meta', name = 'ocr-'..meta, content = t}
end


function hocr.get_title_tag(header, default)
    local t = header.title
    if not t then
        t = default
    end
    return {tag = 'title', t}
end


function hocr.escape_hocr_char(c --[[integer]], bbox)
    if c == string.byte('&') then
        return '&amp;'
    elseif c == string.byte('<') then
        return '&lt;'
    elseif c == string.byte('>') then
        return '&gt;'
    elseif c > 127 then
        return ('&#%d;'):format(c)
    else
        return string.char(c)
    end
end


function hocr.dump(file, dom, preamble)
    if preamble then
        file:write(preamble)
    end
    if dom.tag then
        file:write('<')
        file:write(dom.tag)
        -- dump attributes
        for key, value in pairs(dom) do
            if type(key) == 'string' and key ~= 'tag' then
                file:write((' %s="%s"'):format(key, value:gsub('"', '\\"')))
            end
        end
        if #dom == 0 then
            -- close the tag (short form)
            file:write('/')
        end
        file:write('>')
    end
    -- dump contents
    for key, value in ipairs(dom) do
        if type(value) == 'string' then
            file:write(value)
        else
            hocr.dump(file, value)
        end
    end
    -- close the tag (long form)
    if dom.tag and #dom > 0 then
        file:write('</')
        file:write(dom.tag)
        file:write('>')
    end
end

-------------------------------------------------------------------------------

hocr.preamble = [[<!DOCTYPE html
    PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN
    http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
]]


function hocr.get_html_tag()
    return {tag = 'html', xmlns = 'http://www.w3.org/1999/xhtml'}
end


function hocr.get_head_tag(header)
    header = header or {}
    local tag = {tag = 'head'}
    --get_meta_tag(header, 'system', 'OCRopus' .. get_version_string())
    table.insert(tag, hocr.get_meta_tag(header, 'capabilities', 'ocr_line ocr_page'))
    table.insert(tag, hocr.get_meta_tag(header, 'langs', 'en'))
    table.insert(tag, hocr.get_meta_tag(header, 'scripts', 'Latn'))
    table.insert(tag, hocr.get_meta_tag(header, 'microformats', ''))
    table.insert(tag, hocr.get_title_tag(header, 'OCR Output'))
    return tag
end


function hocr.get_page_DOM(page, image_path)
    local page_bbox
    if page.width and page.height then
        page_bbox = rectangle(0, 0, page:width(), page:height())
    end
    return {tag = 'div', class = 'ocr_page',
                title = hocr.properties_attribute{
                            image = image_path, 
                            bbox = hocr.bbox_to_string(page, page_bbox)}}
end


function hocr.escape(line, remove_hyphens)
    result = ''
    for i = 0, line:length() - 2 do
        result = result..hocr.escape_hocr_char(line:at(i):ord())
    end 
    if remove_hyphens 
        and line:length() > 0 
        and line:at(line:length() - 1):ord() == string.byte('-')
    then
        return result
    else 
        if line:length() > 0 then
            result = result..hocr.escape_hocr_char(line:at(line:length()-1):ord())
        end
        result = result..'\n'
    end
    return result
end


function hocr.get_line_DOM(bbox, text, bboxes, page)
    local props = {bbox = hocr.bbox_to_string(page, bbox)}
    if bboxes then
        props.bboxes = hocr.bboxes_to_string(page, bboxes)
    end
    local tag = {tag = 'span',
                 class = 'ocr_line',
                 title = hocr.properties_attribute(props),
                 hocr.escape(text, remove_hyphens)
                 }
    return tag
end

function hocr.parse_properties(s)
    if not s then return {} end
    local result = {}
    for i in s:gmatch('[^;]+') do
        local key, value = i:match('^%s*([%w]*)%s+(.*)')
        result[key] = value:gsub('%s*$','')
    end
    return result
end

--- Construct an attribute string for hOCR properties.
--- Returns the attribute value (something like 'bbox 10 20 30 40') or an empty string.
function hocr.properties_attribute(properties)
    local s = ''
    for key, value in pairs(properties) do
        if value then
            value = value:gsub('"', '\\"')
            if s == '' then
                s = ('%s %s'):format(key, value)
            else
                s = ('%s; %s %s'):format(s, key, value)
            end
        end
    end
    return s
end

--- Construct an attribute string for hOCR properties.
--- Returns either something like ' title="..."' or an empty string.
function hocr.properties_string(properties)
    local s = hocr.properties_attribute(properties)
    if s == '' then
        return ''
    else
        return (' title="%s"'):format(s)
    end
end

function hocr.find_lines(page_DOM, page_image)
    local h = page_image:dim(1)
    local indices = {}
    local bboxes = {}
    for key, value in ipairs(page_DOM) do
        if value.class == 'ocr_line' then
            table.insert(indices, key)
            props = hocr.parse_properties(value.title)
            table.insert(bboxes, hocr.parse_rectangle(props.bbox, h))
        end
    end
    return indices, bboxes
end

function hocr.find_lines_recursive(indices, bboxes, page_DOM, page_image)
    local h = page_image:dim(1)
    for key, value in ipairs(page_DOM) do
        if value.class == 'ocr_line' then
            table.insert(indices, key)
            props = hocr.parse_properties(value.title)
            table.insert(bboxes, hocr.parse_rectangle(props.bbox, h))
        end
        if type(value) == 'table' then
            hocr.find_lines_recursive(indices, bboxes, value, page_image)
        end
    end
    return indices, bboxes
end

function hocr.copy_DOM_node(node)
    local new_node = {}
    for key, value in pairs(node) do
        if type(key) ~= 'number' then
            new_node[key] = value
        end
    end
    return new_node
end
