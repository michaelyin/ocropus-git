--------------------------------------------------------------------------------
-- Alignment

require 'lib.xml'
require 'lib.path'
require 'lib.datasets'

align = {}

local function crop_and_align(aligner, image, bbox, transcript)
    local line_image = bytearray()
    local margin = 2
    extract_subimage(line_image, image, bbox.x0 - margin, bbox.y0 - margin,
                                        bbox.x1 + margin, bbox.y1 + margin)
    return align.align_line(aligner, line_image, transcript)
end

function align.align_line(aligner, line_image, transcript)
    remove_neighbour_line_components(line_image)
    local chars = nustring.nustring()
    local result = intarray()
    local fst = datasets.transcript_to_fst(transcript)
    local costs = floatarray()
    aligner:align(chars, result, costs, line_image, fst)
    if chars:length() == 0 then
        print('[FAILURE] ' .. transcript:utf8())
    else
        print(chars:utf8())
    end
    return result, costs
end


local function align_word_by_DOM(aligner, output_dir, image, line_DOM, line_no)
    local line_props = parse_hocr_properties(line_DOM.title)
    local bbox = parse_rectangle(line_props.bbox, image:dim(1))
    local transcript = xml.get_transcript_by_line_DOM(line_DOM)
    local seg, line = crop_and_align(aligner, image, bbox,
                                     nustring(transcript))
    if seg and transcript then
        make_line_segmentation_white(seg)
        local line_basename = ('%s/%04d'):format(output_dir, line_no)
        write_png_rgb(line_basename .. '.cseg.png', seg)
        write_png(line_basename .. '.png', line)
        f = io.open(line_basename .. '.txt', 'w')
        f:write(transcript)
        f:write('\n')
        f:close()
    end
end

local function align_page_by_DOM(aligner, hocr_path, output_dir, page_DOM, page_no)
    print(('------------------ PAGE %04d ----------------'):format(page_no))
    local page_dir = string.format('%s/%04d', output_dir, page_no)
    local page_props = hocr.parse_properties(page_DOM.title)
    local img_rel_path = page_props.image
    if not img_rel_path then
        img_rel_path = string.format('Image_%04d.JPEG', page_no)
    end
    os.execute(('mkdir -p "%s"'):format(page_dir))
    local img_path = path.join(path.dirname(hocr_path), img_rel_path)
    local image = util.read_image_gray_checked(img_path)
    local lines = xml.get_list_of_subitems_by_DOM(page_DOM, 'ocr_cinfo')
    for j = 1, #lines do
        align_word_by_DOM(aligner, page_dir, image, lines[j], j)
    end
end

--- Align a document given the hOCR with the ground truth.
--- Alignment is getting character-level data for training.
--- The image names are either taken from hOCR
--- or, if hOCR lacks them, assumed to be 'Image_####.JPEG'.
--- Requires external program `tidy'.
--- @param aligner An IRecognizeLine that can do align().
--- @param hocr_path Path to ground truth in hOCR format.
--- @param output_dir The directory to put aligned lines.
function align.align_book(aligner, hocr_path, output_dir, pages_list)
    print('=============================================')
    print('FORCED ALIGNMENT')
    print('source hOCR: '..hocr_path)
    print('destination: '..output_dir)
    local f = io.popen(string.format('tidy -q -asxml -utf8 "%s"', hocr_path))
    local hocr = xml.collect(f:read('*a'))
    f:close()
    local pages = xml.get_list_of_subitems_by_DOM(hocr, 'ocr_page')
    if type(pages_list) == 'function' then
        pages_list = pages_list(#pages)
    end
    print(pages_list[1], #pages)
    if pages_list then
        for key, i in pairs(pages_list) do
            align_page_by_DOM(aligner, hocr_path, output_dir, pages[i], i - 1)
        end
    else
        for i = 1, #pages do
            align_page_by_DOM(aligner, hocr_path, output_dir, pages[i], i - 1)
        end
    end
end

