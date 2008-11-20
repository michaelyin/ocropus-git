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
-- File: reflow.lua
-- Purpose: split the image into words for reflowing
-- Responsible: mezhirov, faisal
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


require 'lib.util'
require 'lib.headings'
require 'lib.paragraphs'
require 'lib.hocr'

opt,arg = util.getopt(arg)

function option(name, default)
    return opt[name] or os.getenv(name) or default
end

if #arg ~= 2 then
    print("Usage: ocroscript "..arg[0].."input.png output.html")
    os.exit(1)
end
input_path = arg[1]
output_path = arg[2]
images_dir = arg[2]..'_files'
os.execute('mkdir -p "'..images_dir..'"')
images_counter = 0

--- Saves an image in the images directory and returns the tag.
--- Only 'src' attribute is set, you can pass more in additional_attributes.
local function img_tag(image, additional_attributes)
    local src = images_dir .. ('/%04d.png'):format(images_counter)
    images_counter = images_counter + 1
    local img_path = util.combine_paths(output_path, src)
    if tolua.type(image) == 'intarray' then
        write_image_packed(img_path, image)
    else
        write_image_gray(img_path, image)
    end
    local result = {tag = 'img', src = src}
    if additional_attributes then
        for key, value in pairs(additional_attributes) do
            result[key] = value
        end
    end
    return result
end

local function get_images_DOM(tiseg_image, html_path, page_image)
    local rects = rectarray()
    get_nontext_boxes(rects, tiseg_image)
    if rects:length() == 0 then
        return
    end
    local dom = {{tag = 'hr', size = '0'}}
    for i = 0, rects:length() - 1 do
        img = bytearray()
        r = rects:at(i)
        print(tolua.type(page_image))
        extract_subimage(img, page_image, r.x0, r.y0, r.x1, r.y1)
        local props = {bbox = hocr.bbox_to_string(page_image, r)}
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
        local tag = img_tag(img, {width=width, height=height,
                                  class = 'ocr_image',
                                  title = hocr.properties_attribute(props)})
        table.insert(link, tag)
        table.insert(dom, link)
        table.insert(dom, '\n')
    end
    return {tag = 'p', dom}
end
log = Logger('reflow')
tesseract_recognizer = make_TesseractRecognizeLine()

local function count_words(gray, mask)
    local s = nustring()
    local fst = make_StandardFst()
    tesseract_recognizer:recognizeLine(fst, gray)
    fst:bestpath(s)
    s = s:utf8()
    local result = 0
    for i in s:gmatch('%S+') do
        result = result + 1
    end
    return result
end


local function get_reflow_page_DOM(page_seg, page_gray, word_segmenter)
    local r = RegionExtractor()
    r:setPageLines(page_seg)
    local dom = {}
    local line_mask = bytearray()
    local line_gray = bytearray()
    for i = 1, r:length() - 1 do
        -- we've got a line, let's split in into words
        r:extract(line_gray, page_gray, i)
        r:mask(line_mask, i)
        make_background_white(line_mask) -- treat the mask like an image
        local wordseg = intarray()
        --word_segmenter:segment(wordseg, line_mask) 
        local nwords = count_words(line_gray, line_mask)
        segment_words_by_projection(wordseg, line_mask, nwords)
        make_line_segmentation_black(wordseg)
        local wr = RegionExtractor()
        --simple_recolor(wordseg)
        log:recolor('wordseg', wordseg)
        log:log('line_gray', line_gray)
        wr:setImage(wordseg)
        local line_info = TextLineExtended()
        line_info.bbox = rectangle(0, 0, line_gray:dim(0), line_gray:dim(1))
        get_extended_line_info_using_ccs(line_info, line_mask);
        line_DOM = hocr.get_line_DOM(r:bbox(i), nustring(''), nil, page_gray)
        --painted_line = intarray()
        --narray.copy(painted_line, line_gray)
        --replace_values(painted_line, 255, 0xFFFFFF)
        --paint_line(painted_line, line_info)
        for j = 1, wr:length() - 1 do
            local word_gray = bytearray() --intarray()
            --wr:extract(word_gray, line_gray, j)
            extract_subimage(word_gray, line_gray, wr:bbox(j).x0, 0, wr:bbox(j).x1, wr:bbox(j).y1)
            local baseline = line_info.c + line_info.m * wr:bbox(j).x0
            local xheight = line_info.x
            word_height = word_gray:dim(1)
            line_height = line_gray:dim(1)
            max_xheight = 0 + option('max-xheight', 1e30)
            if xheight > max_xheight then
                local coef = max_xheight / xheight
                line_height = line_height * coef
                word_height = word_height * coef
                baseline = baseline * coef
                xheight = max_xheight
                local temp_image = bytearray()
                rescale_to_height(temp_image, word_gray, word_height)
                word_gray = temp_image
            end

            table.insert(line_DOM, img_tag(word_gray, 
                    {style='position: relative; top: '..math.floor(baseline)..'px',
                     height=''..word_height}))
            table.insert(line_DOM, {' ', tag='span',
                                style='font-size: '..line_height..'px'})
        end
        table.insert(dom, line_DOM)
    end
    return dom
end

set_version_string(hardcoded_version_string())

segmenter = make_SegmentPageByRAST()
cleaner = make_DocClean()
tiseg = make_TextImageSegByLeptonica()
word_segmenter =make_SegmentWords()
deskewer = make_DeskewPageByRAST()
binarizer = make_BinarizeByRange()

page_gray = bytearray()
page_binary = bytearray()
clean_image = bytearray()
deskewed_image = bytearray()
tiseg_image = intarray()
nontext_mask = bytearray()
text_image = bytearray()
page_segmentation = intarray()


body_DOM = {tag = 'body'}
pages = Pages()
pages:parseSpec(input_path)
while pages:nextPage() do
    pages:getGray(page_gray)
    deskewer:cleanup(deskewed_image, page_gray)
    binarizer:binarize(page_binary, deskewed_image)
    cleaner:cleanup(clean_image, page_binary)
    tiseg:textImageProbabilities(tiseg_image, clean_image)
    get_nontext_mask(nontext_mask,tiseg_image)
    remove_masked_region(text_image,nontext_mask,clean_image)
    segmenter:segment(page_segmentation,text_image)

    page_DOM = get_reflow_page_DOM(page_segmentation, deskewed_image, word_segmenter)

    page_DOM = detect_headings(page_DOM, page_binary)
    page_DOM = detect_paragraphs(page_DOM, page_binary)

    images_DOM = get_images_DOM(tiseg_image, output_path, deskewed_image)
    table.insert(page_DOM, images_DOM)
    table.insert(body_DOM, page_DOM)
end

doc_DOM = hocr.get_html_tag()
table.insert(doc_DOM, hocr.get_head_tag())
table.insert(doc_DOM, '\n')
table.insert(doc_DOM, body_DOM)

output = io.open(output_path, 'w')
hocr.dump(output, doc_DOM, hocr.preamble)
output:close()
