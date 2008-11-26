import_all(iulib)
import_all(ocr)
import_all(layout)

if #arg < 2 then
    print("usage: ... input output")
    os.exit(1)
end

proc = make_DeskewGrayPageByRAST()

input = bytearray:new()
output = bytearray:new()
read_image_gray(input,arg[1])
proc:cleanup(output,input)
write_png(arg[2],output)
