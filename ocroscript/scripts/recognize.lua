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
-- File: recognize.lua
-- Purpose: top-level recognition script
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org


require 'lib.util'
require 'lib.path'
require 'lib.headings'
require 'lib.hocr'
require 'lib.paragraphs'
import_all(ocr)
import_all(graphics)

--------------------------------------------------------------------------------

opt,arg = util.getopt(arg)

function option(name, default)
    return opt[name] or os.getenv(name) or default
end

output_mode = option('output-mode', 'hocr')
hocr_output = output_mode == 'hocr'
bpnet_path = util.get_bpnet_path(opt)
use_tesseract = not bpnet_path

langmod_path = option('langmod', nil)
if langmod_path then
    langmod = make_StandardFst()
    langmod:load(langmod_path)
end

if #arg == 0 then
    print("Usage: ocroscript "..arg[0].." [options] input.png ... >output.hocr")
    print "Options:"
    print "    --tesslanguage=...   Set recognition language for Tesseract"
    print "    --bpnet=<path>       Use a given neural net instead of Tesseract"
    print "    --bpnetpath=<path>   A path to search for the neural net"
    print "    --output-mode=(hocr|text)   Set output format (default: hOCR)"
    print "    --charboxes          Output bounding boxes for characters (Tesseract only)"
    print "    --langmod=...        Use the given FST as language model"
    os.exit(1)
end

remove_hyphens = true

if use_tesseract then
    if not tesseract then
        print "Compiled without Tesseract support, can't continue."
        os.exit(1)
    end
    tesseract.init(option("tesslanguage", "eng"))
end

--------------------------------------------------------------------------------


set_version_string(hardcoded_version_string())

local function narray_to_table(a)
    local result = {}
    for i = 0, a:length() - 1 do
        table.insert(result, a:at(i))
    end
    return result
end

-- RecognizedPage is a transport object of tesseract.recognize_blockwise().
-- This function will convert it to a DOM.
function convert_RecognizedPage_to_DOM(p, image_path, keep_char_boxes)
    page_DOM = hocr.get_page_DOM(p, image_path)
    for i = 0, p:linesCount() - 1 do
        local bbox = p:bbox(i)
        assert(bbox.x0 >= 0 and bbox.y0 >= 0, "recognized bboxes are wrong")
        local text = nustring()
        local bboxes
        if keep_char_boxes then
            local r = rectarray:new() -- FIXME: there's a memory problem!!!
            p:bboxes(r, i)
            bboxes = narray_to_table(r)
        end
        p:text(text, i)
        line_DOM = hocr.get_line_DOM(bbox, text, bboxes, p)
        table.insert(page_DOM, line_DOM)
    end
    return page_DOM
end

--------------------------------------------------------------------------------

function rec_tesseract(page_gray, page_image, page_seg, pages)
    local page_DOM
    local p = tesseract.RecognizedPage()
    tesseract.recognize_blockwise(p, page_image, page_seg)
    if hocr_output then
        page_DOM = convert_RecognizedPage_to_DOM(p, pages:getFileName(), 
                                                 option("charboxes"))
    else
        page_DOM = {}
        for i = 0, p:linesCount() - 1 do
            local s = nustring()
            p:text(s, i)
            table.insert(page_DOM, s)
        end
    end
    return page_DOM
end

--------------------------------------------------------------------------------

function rec_bpnet(page_gray, page_binary, page_seg, pages, bpnet)
    local regions = RegionExtractor()
    regions:setPageLines(page_seg)
    if hocr_output then
       page_DOM = hocr.get_page_DOM(page_binary, pages:getFileName())
    else
        page_DOM = {}
    end

    for i = 1,regions:length()-1 do
        line_image = bytearray()
        regions:extract(line_image,page_gray,i,1)
        dshow(line_image,"Yyy")

        fst = make_StandardFst()
        bpnet:recognizeLine(fst, line_image)
        -- local pruned = openfst.fst.StdVectorFst()
        -- openfst.fst_prune_arcs(pruned,fst,4,5.0,true)
        -- fst = pruned
        local result = nustring()

        if langmod then
            beam_search_in_composition(result, fst, langmod)
        else
            beam_search(result, fst)
        end
        if hocr_output then
            line_DOM = hocr.get_line_DOM(regions:bbox(i), result, 
                                         nil, page_binary)
        else
            line_DOM = result
        end
        table.insert(page_DOM, line_DOM)
    end
    return page_DOM
end

--------------------------------------------------------------------------------

if bpnet_path then
    bpnet = make_NewBpnetLineOCR(bpnet_path)
end
segmenter = make_SegmentPageByRAST()
page_gray = bytearray()
page_binary = bytearray()
page_seg = intarray()
body_DOM = {tag = 'body'}
for i = 1, #arg do
    pages = Pages()
    pages:parseSpec(arg[i])
    while pages:nextPage() do
        pages:getBinary(page_binary)
        pages:getGray(page_gray)
        segmenter:segment(page_seg, page_binary)
        if bpnet then
            page_DOM = rec_bpnet(page_gray, page_binary, page_seg, pages, bpnet)
        else
            page_DOM = rec_tesseract(page_gray, page_binary, page_seg, pages)
        end
        if hocr_output then
            page_DOM = detect_headings(page_DOM, page_binary)
            page_DOM = detect_paragraphs(page_DOM, page_binary)
        end
        table.insert(body_DOM, page_DOM)
    end
end

if hocr_output then
    doc_DOM = hocr.get_html_tag()
    table.insert(doc_DOM, hocr.get_head_tag())
    table.insert(doc_DOM, '\n')
    table.insert(doc_DOM, body_DOM)
    hocr.dump(io.stdout, doc_DOM, hocr.preamble)
    print()
else
    for _, page in ipairs(body_DOM) do
        for __, line in ipairs(page) do
            print(line:utf8())
        end
    end
end
