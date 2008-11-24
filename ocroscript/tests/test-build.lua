-- test for presence of certain items necessary for recognize.lua

assert(nustring)
assert(bytearray)
assert(intarray)
assert(ocr.set_version_string)
assert(ocr.hardcoded_version_string)
assert(pfst.make_StandardFst)
assert(ocr.make_SegmentPageByRAST)
assert(ocr.Pages)
assert(ocr.RegionExtractor)
if tesseract then
    assert(tesseract.RecognizedPage)
    assert(tesseract.recognize_blockwise)
end
