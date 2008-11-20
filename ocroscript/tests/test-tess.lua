if not tesseract then
    print "Tesseract is disabled, we can't test it."
    os.exit(0)
end

-- check whether tesseract is still working correctly

dofile("utest.lua")

note("trying out tesseract on a block of simple text")

expected = [[This is a lot of 12 point text to test the
ocr code and see if it works on all types
of file format.
The quick brown dog jumped over the
lazy fox. The quick brown dog jumped
over the lazy fox. The quick brown dog
jumped over the lazy fox. The quick
brown dog jumped over the lazy fox.

]]

image = bytearray()
iulib.read_image_gray(image,"images/simple.png")
iulib.make_page_black(image)
ocr.invert(image)
tesseract.init("eng")
result = tesseract.recognize_block(image)
test_assert(expected==result,"tesseract block recognizer")
if expected == result then
    la = ocr.make_SegmentPageByRAST()
    seg = intarray()
    la:segment(seg, image)
    p = tesseract.RecognizedPage()
    tesseract.recognize_blockwise(p, image, seg);
    if p:linesCount() == 0 then
        print "OCRopus can't get bounding boxes from Tesseract."
        print "This symptom is typical for Tesseract 2.03 unless patched."
        print "Please see INSTALL file in OCRopus release directory"
        print "for instructions on applying the patch."
    end
    test_assert(p:linesCount() == 8)
end
tesseract.finish()
