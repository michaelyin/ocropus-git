require 'lib.util'
require 'lib.path'

import_all(ocr)
import_all(nustring)

datasets = {}

--------------------------------------------------------------------------------
-- Transcript files

--- Return a table of nustrings that correspond
--- to characters in the segmentation.
--- This function takes a string as might be read from a transcript file.
--- @param s A Lua string with the transcript.
function datasets.parse_transcript(s)
    local result = {}
    if s:find('\t') then
        -- tab-separated transcript
        for i in s:gmatch('[^\t]+') do
            table.insert(result, nustring(i))
        end
    else
        -- usual transcript (one unicode char per glyph)
        local t = nustring(s)
        for i = 0, t:length() - 1 do
            s = nustring()
            s:push(t:at(i))
            result[i+1] = s
        end
    end
    return result
end

--- Read a transcript from a file and return an array of nustrings.
--- @param path Path to the file that contains only one line: the transcript.
function datasets.read_transcript(path)
    local stream = assert(io.open(path))
    local transcript = datasets.parse_transcript(stream:read("*a"):gsub('\n$',''))
    stream:close()
    return transcript
end

--------------------------------------------------------------------------------
function datasets.append_nustring_to_alignment_fst(fst, string)
    local extra_space_cost = 10
    local s = fst:getStart()
    for j = 0, string:length() - 1 do
        local c = string:at(j):ord()
        if c == 32 then
            -- add a loop to the current state
            fst:addTransition(s, s, 32, 0, 32)
        else
            -- add an arc to the next state
            local t = fst:newState()
            fst:addTransition(s, s, 32, extra_space_cost, 0)
            fst:addTransition(s, t, c, 0, c)
            s = t
        end
    end
    return s
end

function datasets.transcript_to_fst(transcript)
    assert(transcript)
    local fst = pfst.make_StandardFst()
    tolua.takeownership(fst) -- FIXME
    local s = fst:newState()
    fst:setStart(s)
    if type(transcript) == 'table' then
        for i = 1, #transcript do
            s = datasets.append_nustring_to_alignment_fst(fst, transcript[i])
        end
    else
        s = datasets.append_nustring_to_alignment_fst(fst, transcript)
    end
    fst:setAccept(s)
    return fst
end


--- Read a word-by-word ground truth
-- (such files are produced by parse-google-hocr.py)
-- @param path Path to the file containing lines of this form: x0 y0 x1 y1 word.
-- @param h Height (required to convert boxes to ocropus's
--                  lower origin convention).
-- @return The list of {x0,y0,x1,y1,gt} tables and a whole line as a string.
function datasets.read_words(path, h)
    local transcript = ''
    local words = {}
    for line in io.lines(path) do
        local x0, y0, x1, y1, gt = line:match(
            '^(%d+)%s*(%d+)%s*(%d+)%s*(%d+)%s*(.*)$')
        assert(x0 and y0 and x1 and y1 and gt, "ground truth parsing error")
        -- convert coordinates to numbers and pack into a table
        table.insert(words, {x0=x0+0, y0=h-1-y1, x1=x1+0, y1=h-1-y0, gt=gt})
        transcript = transcript .. ' ' .. gt
    end
    return words, transcript:gsub('^ ','')
end

--- Read a main file of a standard OCRopus dataset format
--- (a list of image and transcript filenames, possibly segmentation filename)
--- and iterate over the filenames.
--- Also supports the list-of-basenames format.
local function dataset_filenames_list_style(p)
    local dirname = path.dirname(p)
    return function(file)
        while true do
            local s = file:read("*l")
            if not s then
                return
            end
            local image_path, text_path, seg_path = 
                s:match("^%s*([^%s]+)%s+([^%s]+)%s+([^%s]+)%s*$")
            if image_path and text_path and seg_path then
                return path.join(dirname, image_path),
                       path.join(dirname, text_path),
                       path.join(dirname, seg_path)
            end
            image_path, text_path = s:match("^%s*([^%s]+)%s+([^%s]+)%s*$")
            if image_path and text_path then
                return path.join(dirname, image_path),
                       path.join(dirname, text_path)
            end
            image_path = s:match("^%s*([^%s]+)%s*$")
            if image_path then
                local basename = path.join(dirname, image_path)
                return basename..'.png',
                       basename..'.txt',
                       basename..'.cseg.png'
            end
        end
    end, io.open(p, "r")
end

local function dataset_filenames_ocrvis_style(path)
    -- TODO: union of several directories
    return function(file)
        local s = file:read('*l')
        if not s then
            return
        end
        return s,
               s:gsub('[.]png$', '.txt'),
               s:gsub('[.]png$', '.cseg.png')
    end, io.popen('ls -1 "'..path..'"/Volume_????/????/????.png')
end

function datasets.dataset_filenames(path)
    if os.execute('[ -d "'..path..'" ]') == 0 then
        -- we have a directory, let's use ocrvis format
        return dataset_filenames_ocrvis_style(path)
    else
        return dataset_filenames_list_style(path)
    end
end


--- Iterate over (image, text) entries given a list of (image, text) filenames.
--- @param mode A flag: "color" means that images should be read in color.
function datasets.dataset_entries(path, mode)
    local log = Logger('dataset')
    iterator, file = datasets.dataset_filenames(path)
    return function()
        local image_path, text_path, seg_path = iterator(file)
        log:log('image_path', image_path)
        log:log('text_path', text_path)
        log:log('seg_path', seg_path)
        local onlyseg_mode = (mode == 'onlyseg') and not seg_path
        if not image_path then
            return
        else
            if mode == 'onlytext' then
                return true, datasets.read_transcript(text_path)
            end
            local image
            if mode == "color" or onlyseg_mode then
                image = util.read_image_color_checked(image_path)
            else
                image = util.read_image_gray_checked(image_path)
            end
            local seg
            if onlyseg_mode then
                seg = image
                log:log('seg', seg)
                image = bytearray()
                forget_segmentation(image, seg)
            elseif seg_path then
                seg = util.read_image_color_checked(seg_path)
                log:log('seg', seg)
            end
            log:log('image', image)
            local transcript = datasets.read_transcript(text_path)
            return image, transcript, seg, image_path
        end
    end
end

function datasets.log_dataset(logger, path, mode)
    for image, text in dataset_entries(path, mode) do
        logger:log("image", image)
        for i = 1, #text do
            logger:log(("char[%d]"):format(i), text[i])
        end
    end
end


