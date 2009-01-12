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
-- File: rec-tess.lua
-- Purpose: recognition through Tesseract
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


require 'lib.util'
require 'lib.headings'
require 'lib.paragraphs'

remove_hyphens = true

if not tesseract then
    print "Compiled without Tesseract support, can't continue."
    os.exit(1)
end

opt,arg = util.getopt(arg)

function option(name, default)
    return opt[name] or os.getenv(name) or default
end

function print_usage_and_exit()
    print("Usage: ocroscript "..arg[0].." [--tesslanguage=...] input.png output.hocr")
    os.exit(1)
end

if #arg == 0 then
    print_usage_and_exit()
end

tesseract.init(option("tesslanguage", "eng"))
set_version_string(hardcoded_version_string())

cleaner = make_DocClean()
tiseg = make_TextImageSegByLeptonica()
segmenter = make_SegmentPageByRAST()

page_image = bytearray()
clean_image = bytearray()
tiseg_image = intarray()
nontext_mask = bytearray()
text_image = bytearray()
page_segmentation = intarray()

local function narray_to_table(a)
    local result = {}
    for i = 0, a:length() - 1 do
        table.insert(result, a:at(i))
    end
    return result
end


-- RecognizedPage is a transport object of tesseract_recognize_blockwise().
-- This function will convert it to a DOM.
function convert_RecognizedPage_to_DOM(p, image_path, keep_char_boxes)
    page_DOM = get_page_DOM(p, image_path)
    for i = 0, p:linesCount() - 1 do
        local bbox = p:bbox(i)
        local text = nustring()
        local bboxes
        if keep_char_boxes then
            local r = rectarray:new() -- FIXME: there's a memory problem!!!
            p:bboxes(r, i)
            bboxes = narray_to_table(r)
        end
        p:text(text, i)
        line_DOM = get_line_DOM(bbox, text, bboxes, p)
        table.insert(page_DOM, line_DOM)
    end
    return page_DOM
end


function get_images_DOM(tiseg_image, html_path, images_dir, page_image)
    os.execute('mkdir -p "'..images_dir..'"')
    local rects = rectarray()
    get_nontext_boxes(rects, tiseg_image)
    if rects:length() == 0 then
        return
    end
    local dom = {{tag = 'hr', size = '0'}}
    for i = 0, rects:length() - 1 do
        local src = images_dir .. ('/%04d.png'):format(i + 1)
        local img_path = util.combine_paths(html_path, src)
        img = bytearray()
        r = rects:at(i)
        extract_subimage(img, page_image, r.x0, r.y0, r.x1, r.y1)
        write_image_gray(img_path, img)
        local props = {bbox = bbox_to_string(page_image, r)}
        local link = {tag = 'a', href=src}
        local width = r.x1 - r.x0
        local height = r.y1 -r.y0
        if width > height then
           width = "200px"
           height = "auto"
        else
           width = "auto"
           height = "200px"
        end
        local tag = {tag = 'img', src = src, width=width, height=height,
                     class = 'ocr_image', title = hocr_properties_attribute(props)}
        table.insert(link, tag)
        table.insert(dom, link)
        table.insert(dom, '\n')
    end
    return {tag = 'p', dom}
end

if #arg ~= 2 then
    print_usage_and_exit()
end

input_file = arg[1]
output_file = arg[2]

body_DOM = {tag = 'body'}
--for i = 1, #arg do
pages = Pages()
pages:parseSpec(input_file)
while pages:nextPage() do
    pages:getBinary(page_image)
    binary_invert(page_image)
    cleaner:cleanup(clean_image,page_image)
    tiseg:textImageProbabilities(tiseg_image,clean_image)
    get_nontext_mask(nontext_mask,tiseg_image)
    remove_masked_region(text_image,nontext_mask,clean_image)
    segmenter:segment(page_segmentation,text_image)
    local p = RecognizedPage()
    tesseract_recognize_blockwise(p, page_image, page_segmentation)
    page_DOM = convert_RecognizedPage_to_DOM(p, pages:getFileName(), 
                                             option("charboxes"))
    page_DOM = detect_headings(page_DOM, page_image)
    page_DOM = detect_paragraphs(page_DOM, page_image)
    table.insert(page_DOM, get_images_DOM(tiseg_image, output_file, output_file..'_files', page_image))

    table.insert(body_DOM, page_DOM)
end
--end
doc_DOM = get_html_tag()
table.insert(doc_DOM, get_head_tag())
table.insert(doc_DOM, '\n')
table.insert(doc_DOM, body_DOM)
file = io.open(output_file, 'w')
dump_DOM(file, doc_DOM, html_preamble)
file:close()
