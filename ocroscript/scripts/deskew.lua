-- FIXME this should do the right thing for grayscale images:
-- threshold, compute rotation, then rotate the grayscale image

if #arg < 2 then
    print("usage: ... input output")
    os.exit(1)
end

proc = make_DeskewPageByRAST()

input = bytearray:new()
output = bytearray:new()
read_image_gray(input,arg[1])
proc:cleanup(output,input)
write_png(arg[2],output)
