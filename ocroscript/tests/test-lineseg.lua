require 'lib.util'

function overseg_is_sane(overseg)
    local b = bytearray()
    ocr.forget_segmentation(b, overseg)
    local concomp_seg = intarray()
    narray.sub(b, 255)
    narray.copy(concomp_seg, b)
    iulib.label_components(concomp_seg)
    return ocr.is_oversegmentation_of(overseg, concomp_seg);
end

segmenter = ocr.make_CurvedCutSegmenter()

function check_line(path)
    local image = util.read_image_gray_checked(path)
    local seg = intarray()
    segmenter:charseg(seg, image)
    assert(overseg_is_sane(seg))
end

check_line('images/line.png')
check_line('images/line-big.png')
check_line('images/line-tiny.png')
