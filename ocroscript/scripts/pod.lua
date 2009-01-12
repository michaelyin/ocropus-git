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

-- outputs the rotated image according to orientation
function rotate(page_image_rot,page_image,orientation)
    if orientation == 0 then
        narray.copy(page_image_rot,page_image)
    elseif orientation == 90 then
        ocr.rotate_90(page_image_rot,page_image)
    elseif orientation == 180 then
        ocr.rotate_180(page_image_rot,page_image)
    elseif orientation == 270 then
        ocr.rotate_270(page_image_rot,page_image)
    else
        os.exit(1)
    end
end

-- give some statistics about the page_image in the specified orientation
-- maxlines is the maximum number of lines to consider
-- page image is a binarized image
function estimate_one_orientation_with_tess(page_image,orientation,maxlines)
    local page_image_rot    = bytearray:new()
    local page_segmentation = intarray:new()

    logger:log("orientation",orientation)                                       -- log
    rotate(page_image_rot,page_image,orientation)                               -- rotate

    local list_trans_scores = {}                                                -- prepare scores
    for i=1,math.max(1,maxlines) do
        list_trans_scores[i] = {"",-1}                                          -- initialize scores
    end

    local regions = RegionExtractor:new()                                             -- prepare regions

    segmenter:segment(page_segmentation,page_image_rot)                         -- segment
    regions:setPageLines(page_segmentation)                                     -- create regions
    logger:log("region length",regions:length())                                -- log

    if regions:length() > 1 then                                               -- it can happen for blank pages
        local tesseract_recognizer = make_TesseractRecognizeLine()              -- recognition by Tesseract
        local bb_len = {}
        for i = 1,regions:length()-1 do                                         -- get the bounding boxes
            table.insert(bb_len,{i,regions:bbox(i):width()})
        end
        table.sort(bb_len,function(a,b) return a[2]>b[2] end)                   -- sort bboxes by width

        local line_image = bytearray:new()                                      -- prepare a line image
        local transcript = nustring():utf8()
        for j=1,math.min(regions:length()-1,maxlines) do                        -- for the specified number of bboxes
            regions:extract(line_image,page_image_rot,bb_len[j][1],1)           -- get the image of the line
            remove_neighbour_line_components(line_image)                        -- clean, maybe not necessary
            logger:log("image for orientation: "..                              -- log
                orientation.." and trial: "..j,line_image)
            local fst = make_StandardFst()                                      -- prepare fst
            tesseract_recognizer:recognizeLine(fst,line_image)                  -- recognize line

            local result = nustring()                                           -- prepare nustring
            fst:bestpath(result)                                                -- get the best path
            transcript = result:utf8()                                          -- get the transcription

            score = score_line(transcript,dictionary)                           -- get the score of the line

            logger:log("score",score)                                           -- log
            str_html = tohtml(transcript)
            logger:log("transcript",str_html)

            list_trans_scores[j] = {transcript,score}                           -- append the score
        end
        line_image:delete()
    end
    regions:delete()                                                            -- new and delete are necessary when
    page_segmentation:delete()                                                  -- dealing with lot of images
    page_image_rot:delete()

    return list_trans_scores
end

function estimate_all_orientations(pages,max_lines_to_compute,max_lines_to_use,mode,path_out)
    local scores = {}
    local page_image = bytearray:new()  -- prepare to store the page image
    local last_orien = -1
    logger:log(pages:getFileName())
    while pages:nextPage() do           -- can have several pages
        pages:getGray(page_image)       -- get the grayscale just for logging
        logger:log("getGray image",page_image,20.)  -- log

        pages:getBinary(page_image) -- the the binarized image
        logger:log("getBinary image",page_image,20.)    --log
        for angle=0,270,90 do       -- for the four orientation
            list_trans_scores = estimate_one_orientation_with_tess(
                page_image,angle,max_lines_to_compute) --estimate
            if mode == "max" then
                -- in this script we are using the maximum score amoung the total number of line
                -- it helps for finding multiple orientation but can also miss the main orientation
                -- using mean or median might be more clever
                argmax = 1
                for j=2,math.min(max_lines_to_use,#list_trans_scores) do
                    if list_trans_scores[j][2] >= list_trans_scores[argmax][2] then
                        argmax=j        -- find the best line
                    end
                end
                table.insert(   scores,{list_trans_scores[argmax][2],
                                angle,list_trans_scores[argmax][1]})    -- store the best line
                logger:log(argmax.."  "..pages:getFileName()..
                    "  "..list_trans_scores[argmax][2].."  "..angle.."  "..
                    tohtml(list_trans_scores[argmax][1]))
            elseif mode == "mean" then
                sum = 0
                for j=1,math.min(max_lines_to_use,#list_trans_scores) do
                    if list_trans_scores[j][2] > 0 then
                        sum = sum + list_trans_scores[j][2]
                    end
                end
                mean = sum/math.max(max_lines_to_use,#list_trans_scores)
                table.insert(scores,{mean,angle,""})
            else
                os.exit(1)
            end
        end
        table.sort(scores, function(a,b) return a[1]>b[1] end)          -- find the best orientation
        print(pages:getFileName().."\t".."orientation="..scores[1][2].."\t"..
            string.format("[score("..scores[1][2]..")=%.2f",scores[1][1])..
            string.format(" score("..scores[2][2]..")=%.2f",scores[2][1])..
            string.format(" score("..scores[3][2]..")=%.2f",scores[3][1])..
            string.format(" score("..scores[4][2]..")=%.2f",scores[4][1]).."]")
        logger:log(pages:getFileName().."  "..scores[1][1].."  "..
            scores[1][2].."  "..tohtml(scores[1][3]))                   -- log
        last_orien = scores[1][2]
        if (path_out ~= nil) then
            local page_image_rot = bytearray:new()
            rotate(page_image_rot,page_image,scores[1][2])
            iulib.write_image_gray(path_out.."/"..path.filename(pages:getFileName()),page_image_rot)
            page_image_rot:delete()
        end
    end
    page_image:delete()
    return last_orien
end

function main()
    local opt,arg = util.getopt(arg)
    if #arg < 1 then
        print 'Usage: ... [options] <basenames/png>'
        print '     outputs the orientation for all the files in <basenames> or single png file'
        print 'Options:'
        print '     --dict=...       - path of a dictionary file for POD. By default, the'
        print '                        English dictionary of OCROpus is read'
        print '     --tessdict=...   - Tesseract dictionary, "eng" by default'
        print '     --maxcom=...     - maximum number of lines to extract with RAST. Set to 4 by default'
        print '     --maxuse=...     - maximum number of lines to use when estimating the orientation. Set to 4 by default'
        print '     --start=...      - start at this position in the basenames'
        print '     --finish=...     - stop working at this position in the basenames'
        print '     --mode=...       - decision mode: max or mean, max by default. Max mode will catch easier multiple orientation'
        print '                        but might miss the main orientation. Mean mode will miss multiple orientation but will be more'
        print '                        robust to detect the main orientation'
        print '     --pathout=...    - folder to output the final binarizations'

        os.exit(1)
    end

    local start_loop  = tonumber(opt.start)       -- start working at that position in the basenames
    local end_loop    = tonumber(opt.finish)      -- stop working at that position in the basenames
    local mode        = opt.mode or "max"

    local max_lines_to_compute = 4
    if (opt.maxcom ~= nil) then                     -- get the number of line to extract and to use
        max_lines_to_compute = tonumber(opt.maxcom)
    end
    local max_lines_to_use = 4
    if (opt.maxuse ~= nil) then
        max_lines_to_use = tonumber(opt.maxuse)
    end
    max_lines_to_use = math.min(max_lines_to_compute,max_lines_to_use)

    tesseract.tesseract_init_with_language(opt.tess or "eng")   -- set Tesseract dictionary
    if (opt.dict ~= nil) then
        read_dict(dictionary,opt.dict)
    else
        read_english_dict(dictionary)
    end

    segmenter:set("max_results",max_lines_to_compute)   -- do not extract everything

    local pages = Pages:new()                       -- use the Pages class for regular processing
    local binarizer = make_BinarizeBySauvola()      -- default Sauvola works well in general
    pages:setBinarizer(binarizer)                   -- just use it
    pages:setAutoInvert(false)                      -- sometimes the auto invert is wrong

    local count = 0
    local last_orien = -1
    if (arg[1]:sub(arg[1]:len()-3):lower()==".png") then
        pages:parseSpec(arg[1])
        last_orien = estimate_all_orientations(pages,max_lines_to_compute,max_lines_to_use,mode,opt.pathout)
    else
        for image_path, text_path, seg_path in datasets.dataset_filenames(arg[1]) do
            if (start_loop ~= nil) and (count < start_loop) then
                --continue
            else
                pages:parseSpec(image_path)
                last_orien = estimate_all_orientations(pages,max_lines_to_compute,max_lines_to_use,mode,opt.pathout)
            end
            if end_loop ~= nil and count >= end_loop then
                break
            end
            count = count+1
        end
    end
    pages:delete()
    return last_orien
end

return main()
