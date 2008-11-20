-- This script alignes a dataset (by a list of basenames) word-by-word.
-- Word segmentation information (.words.png) must be already present.
-- FIXME remove this script; it's not needed

require 'lib.util'
require 'lib.align'
require 'lib.datasets'

import_all(ocr)

default_cost_threshold = 10


if #arg < 2 then
    print("Usage: "..arg[0].." <dataset> <neural net>")
    os.exit(2)
end

local c = make_BpnetClassifier()
local cc = make_AdaptClassifier(c)
cc:load(arg[2])
local seg = make_CurvedCutSegmenter() -- make_ConnectedComponentSegmenter()
local line_ocr = make_NewGroupingLineOCR(cc, seg)

function transcript_to_string(transcript)
    local s = ""
    for i = 1, #transcript do
        s = s..transcript[i]:utf8()
    end
    return s
end

-- taken from train-bpnet-lines
-- FIXME: should go somewhere into lib.util --IM
function concatenate_transcript(transcript)
    -- this is an insane way of concatenating!
    local s = ""
    for i = 1, #transcript do
        s = s..transcript[i]:utf8()
    end
    return nustring(s)
end

function load_transcript_wordwise(path)
    -- on second thought, I should've parsed the transcript before splitting it.
    print(path)
    local file = io.open(path)
    local text = file:read('*a'):gsub('\n$','')
    file:close()

    local tabbed_mode = text:find('\t')
    local words = {}
    for word in text:gmatch('[^ ]+') do
        if tabbed_mode and not word:find('\t') then
            -- we should indicate tabbed mode because it affects parsing
            word = '\t' .. word
        end
        table.insert(words, datasets.parse_transcript(word))
    end
    return words
end

local function align_line(img_path, image, word_transcripts, wordseg,
                          cost_file,
                          cost_threshold)
    if not cost_threshold then
        cost_threshold = default_cost_threshold
    end
    local r = RegionExtractor()
    r:setImage(wordseg)
    if r:length() - 1 ~= #word_transcripts then
        logger:log("Image path",img_path)
        str = ("### LINE LOST: text has %d words, segmentation has %d (%s)"):
              format(#word_transcripts, r:length() - 1, img_path)
        print(str)
        logger:log(str)
        return
    end

    local seg = intarray()
    narray.makelike(seg, wordseg)
    narray.fill(seg, 0)
    for i = 1, #word_transcripts do
        total_words = total_words + 1
        local word_img = bytearray()
        r:extract(word_img, image, i)
        local padding = 3
        iulib.pad_by(word_img, padding, padding, 255)
        --iulib.write_image_gray("bad_word.png", word_img)
        local transcript = concatenate_transcript(word_transcripts[i])

        --remove_neighbour_line_components(line_image)
        local chars = nustring()
        local aligned_word_padded = intarray()
        local fst = datasets.transcript_to_fst(transcript)
        local costs = floatarray()
        line_ocr:align(chars, aligned_word_padded, costs, word_img, fst)
        local aligned_word = intarray()
        iulib.extract_subimage(aligned_word, aligned_word_padded, padding, padding,
                                       aligned_word_padded:dim(0) - padding,
                                       aligned_word_padded:dim(1) - padding)
        if chars:length() == 0 then
            logger:log("Image path",img_path)
            str = '[FAILURE] ' .. transcript:utf8()
            print(str)
            logger:log(str)
            logger:log("word",word_img)
        else
            --print(string.format("%s",chars:utf8()))
        end
        cost = narray.max(costs)
        if cost > cost_threshold then
            logger:log("Image path",img_path)
            str = ("### BAD LINE: max cost in word `%s' is %f"):
                  format(transcript_to_string(word_transcripts[i]), cost)
            print(str)
            logger:log(str)
            logger:log("line",image)
            logger:log("word",word_img)
            total_failures = total_failures + 1
            str = ("%d / %d  words"):format(total_failures,total_words)
            logger:log(str)
            return
        end
        concat_segmentation(seg, aligned_word, r:bbox(i).x0, r:bbox(i).y0)
        for j = 0, costs:length() - 2 do -- don't write accept cost
            cost_file:write(("0 0 %f\n"):format(costs:at(j)))
        end
    end
    return seg
end

local c = make_BpnetClassifier()
local cc = make_AdaptClassifier(c)
cc:load(arg[2])
local seg = make_CurvedCutSegmenter()
local line_ocr = make_NewGroupingLineOCR(cc, seg)

logger = Logger('align.errors')
total_words = 0
total_failures = 0

for img_path, txt_path, seg_path in datasets.dataset_filenames(arg[1]) do
    image = util.read_image_gray_checked(img_path)
    word_transcripts = load_transcript_wordwise(txt_path)
    
    --[[wordseg_path = img_path:gsub('[.]png$', '.words.png')
    if arg[3] then
        cmd = ('%s %s %d %s'):format(arg[3], img_path, #word_transcripts,
                                     wordseg_path)
        print(cmd)
        os.execute(cmd)
    end
    wordseg = util.read_image_color_checked(wordseg_path)]]
    image_gray = util.read_image_gray_checked(img_path)
    image_binary = bytearray()
    binarize_simple(image_binary, image_gray)
    local wordseg = intarray()
    if (segment_words_by_projection(wordseg, image_binary, #word_transcripts)) then

        make_line_segmentation_black(wordseg)

        cost_file = util.secure_open(img_path:gsub('[.]png$', '.costs'), 'w')
        
        seg = align_line(img_path,image, word_transcripts, wordseg, cost_file)
        cost_file:close()
        if seg then
            make_line_segmentation_white(seg)
            iulib.write_image_packed(seg_path, seg)
        end
    else
        logger:log(img_path)
        logger:log("Less gaps than desired no of words:",image_binary)
    end
end
