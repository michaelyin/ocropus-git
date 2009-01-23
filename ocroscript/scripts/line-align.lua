import_all(iulib)
import_all(pfst)
import_all(tesseract)

if #arg ~= 3 then
    error("Usage: ocroscript "..arg[0].." line.png line.txt line.cseg.png")
end

t = make_TesseractRecognizeLine()
image = bytearray()
read_image_gray(image, arg[1])

chars = nustring()
seg = intarray()
costs = floatarray()
fst = make_StandardFst()
t:align(chars, seg, costs, image, fst)

f = io.open(arg[2], "w")
f:write(chars:utf8())
f:close()

simple_recolor(seg)

write_image_packed(arg[3], seg) 
