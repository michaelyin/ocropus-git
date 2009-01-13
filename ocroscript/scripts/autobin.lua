require 'lib.util'
require 'lib.datasets'
require 'dictutils'

import_all(iulib)
import_all(ocr)
import_all(tesseract)
import_all(nustring)
import_all(layout)
import_all(pfst)

logger      = Logger('results')         -- a logger
dictionary  = {}                        -- prepare dictionary
segmenter   = make_SegmentPageByRAST()  -- segmentation by RAST

-- evaluate a binarized line
function evaluate_binarization(line_image_binarized)
    remove_neighbour_line_components(line_image_binarized)          --maybe not necessary
    logger:log("line_image binarized cleaned",line_image_binarized)

    local fst = make_StandardFst()      -- prepare fst
    local tesseract_recognizer = make_TesseractRecognizeLine()  -- a Tesseract line recognizer
    tesseract_recognizer:recognizeLine(fst,line_image_binarized)-- recognize
    local result = nustring()
    fst:bestpath(result)                -- get the results    
    local transcript = result:utf8()    -- and the transcription

    score = score_line(transcript,dictionary)   --  score

    transcript = tohtml(transcript)     -- sort of cast, otherwise tohtml is an object
    logger:log("transcript",transcript) -- log
    logger:log("score",score)
    return score
end

-- apply a binarization on a grey image and evalaute
function try_one_binarization(line_image_grey,w,k)
    local line_image_binarized = bytearray:new()    -- prepare binarized image
    local binarizer = make_BinarizeBySauvola()      -- prepare Sauvola
    binarizer:set("w",w)                            -- set parameters
    binarizer:set("k",k)
    binarizer:binarize(line_image_binarized,line_image_grey)    -- apply Sauvola
    -- the grey image was wider to apply Sauvola with w
    -- crop to obtain the real line and remove its neighborhood
    -- but keep one extra pixel to prevent some drawbacks of remove_neighbour_line_components
    crop(line_image_binarized,rectangle(w/2,w/2,line_image_grey:dim(0)-w/2+2,line_image_grey:dim(1)-w/2+2))
    logger:log("line_image binarized",line_image_binarized)     -- log
    r = evaluate_binarization(line_image_binarized) -- get the score
    line_image_binarized:delete()                   -- free binarized image
    return r
end

-- compare a triplet of {score,w,k}
function compare(a,b)
    if a[1] < b[1] then -- we want best score first
        return false
    end
    if a[1] > b[1] then
        return true
    end
    if a[2] < b[2] then -- we want small W
        return true
    end
    if a[2] > b[2] then
        return false
    end
    if a[3] < b[3] then -- we prefer small K
        return true
    end
    if a[3] > b[3] then
        return false
    end
end

function try_thresholds_on_lines(pages,max_lines_to_compute,max_lines_to_use,path_out)
    local binarizer = make_BinarizeBySauvola()          -- for the first extraction, default Sauvola should be a good trade-off
    pages:setBinarizer(binarizer)                       -- set to pages

    local page_binarized = bytearray:new()              -- prepare grey, binarized and segmentation
    local page_gray = bytearray:new()
    local page_segmentation = intarray:new()

    while pages:nextPage() do                           -- for each page
        pages:getGray(page_gray)                        -- log gray and first binarization
        logger:log("page_gray",page_gray,20)
        pages:getBinary(page_binarized)        
        logger:log("page_binarized",page_binarized,20)

        segmenter:segment(page_segmentation,page_binarized) -- first segmentation

        local regions = RegionExtractor:new()           -- prepare region extractor          
        regions:setPageLines(page_segmentation)
        logger:recolor("page_segmentation",page_segmentation,20)    -- log segmentation
        logger:log("regions:length()",regions:length())
        if regions:length() > 1 then                    -- at least one line
            local tesseract_recognizer = make_TesseractRecognizeLine()-- recognition by Tesseract
            local bb_len = {}                           -- prepare evaluation of the bboxes
            for i = 1,regions:length()-1 do
                table.insert(bb_len,{i,regions:bbox(i):width()})    -- wider are interesting
            end
            table.sort(bb_len,function(a,b) return a[2]>b[2] end)   -- sort by width

            local scores = {}
            local line_image_grey = bytearray:new()     -- prepare a grey line
            for w=10,90,20 do                               -- try different w
                for k=0.1,0.9,0.2 do                        -- try different k
                    sumscores = 0                           -- for the best bboxes
                    for j=1,math.min(regions:length()-1,max_lines_to_compute) do
                        logger:log("bb len width",bb_len[j][2])

                        regions:extract(line_image_grey,    -- extract region in the grayscale image
                            page_gray,bb_len[j][1],w/2+1)   -- but enlarge, to be able to apply Sauvola
                        logger:log("line_image extracted",line_image_grey)

                        score = try_one_binarization(line_image_grey,w,k) -- get the score

                        if j <= max_lines_to_use then       -- cumulate all the scores 
                            if score > 0 then
                                sumscores = sumscores + score
                            end
                        end
                    end
                    table.insert(scores,{sumscores,w,k})    -- record the final score for this couple of parameters
                    logger:log(string.format("w,k = (%g,%g)  s=%g",w,k,sumscores))
                end
            end
            table.sort(scores,compare)      -- sort according score, W, K
            print(pages:getFileName().." "..scores[1][2].." "..scores[1][3].." "..scores[1][1])   -- it's the final result
            line_image_grey:delete()        -- free
            if (path_out ~= nil) then
                binarizer:set("w",scores[1][2])
                binarizer:set("k",scores[1][3])
                binarizer:binarize(page_binarized,page_gray)
                iulib.write_image_gray(path_out.."/"..path.filename(pages:getFileName()),page_binarized)
            end
        else
            print(pages:getFileName().." no line in the page")   -- it can happen, first extraction may fail or just a blank page
        end
        regions:delete()
    end
    page_binarized:delete()
    page_gray:delete()
    page_segmentation:delete()
end

function main()
    local opt,arg = util.getopt(arg)
    if #arg < 1 then
        print 'Usage: ... [options] <basenames/png>'
        print '     outputs the best Sauvola''s parameters for all the files in <basenames>  or single png file'
        print 'Options:'
        print '     --dict=...       - path of a dictionary file for POD. By default, the'
        print '                        English dictionary of OCROpus is read'
        print '     --tessdict=...   - Tesseract dictionary, "dum" by default'
        print '     --maxcom=...     - maximum number of lines to extract with RAST. Set to 5 by default'
        print '     --maxuse=...     - maximum number of lines to use when estimating the orientation. Set to 5 by default'
        print '     --start=...      - start at this position in the basenames'
        print '     --finish=...     - stop working at this position in the basenames'
        print '     --pathout=...    - folder to output the final binarizations'

        os.exit(1)
    end

    local start_loop  = tonumber(opt.start)
    local end_loop    = tonumber(opt.finish)

    local max_lines_to_compute = 5
    if (opt.maxcom ~= nil) then                     -- get the number of line to extract and to use
        max_lines_to_compute = tonumber(opt.maxcom)
    end
    local max_lines_to_use = 5
    if (opt.maxuse ~= nil) then
        max_lines_to_use = tonumber(opt.maxuse)
    end
    max_lines_to_use = math.min(max_lines_to_compute,max_lines_to_use)

    tesseract.tesseract_init_with_language(opt.tess or "dum")   -- set Tesseract with a dummy dictionary
    if (opt.dict ~= nil) then
        read_dict(dictionary,opt.dict)
    else
        read_english_dict(dictionary)
    end
    
    segmenter:set("max_results",max_lines_to_compute)   -- do not extract everything
    
    local pages = Pages:new()                       -- use the Pages class for regular processing
    local binarizer = make_BinarizeBySauvola()      -- default Sauvola works well in general
    pages:setBinarizer(binarizer)                   -- just use it
    pages:setAutoInvert(false)                      -- sometimes the autoinvert is wrong

    local count = 0
    if (arg[1]:sub(arg[1]:len()-3):lower()==".png") then    -- just one image
        pages:parseSpec(arg[1])                     -- init
            try_thresholds_on_lines(pages,max_lines_to_compute,max_lines_to_use,opt.pathout)    -- and try parameters
        else
            for image_path, text_path, seg_path in datasets.dataset_filenames(arg[1]) do        -- for all the files
            if (start_loop ~= nil) and (count < start_loop) then
                --continue
            else
                pages:parseSpec(image_path)         -- init
                try_thresholds_on_lines(pages,max_lines_to_compute,max_lines_to_use,opt.pathout)    -- and try parameters
            end
            if end_loop ~= nil and count >= end_loop then
                break
            end
            count = count+1
        end
    end
    pages:delete()
end

main()
