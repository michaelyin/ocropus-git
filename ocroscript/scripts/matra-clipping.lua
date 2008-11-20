min_width = 10 -- minimum width of matra lines
min_height = 8 -- minimum height of vertical lines causing interruption
clip_offset = 3 -- how far to offset the clipping from the vertical line

-- read the input image and invert (black background)
image = bytearray()
read_image_binary(image,arg[1])
binary_invert(image)

-- find horizontal lines
matra = bytearray()
narray.copy(matra,image)
binary_open_rect(matra,min_width,1)

-- find vertical lines
vert = bytearray()
narray.copy(vert,image)
binary_open_rect(vert,1,min_height)

-- find places where horizontal and vertical lines intersect
binary_and(matra,vert,0,0)

-- shift the intersection points by clip_offset and remove
binary_dilate_rect(matra,1,0)
binary_invert(matra)
binary_and(image,matra,clip_offset,0)

-- write out the result
binary_invert(image)
write_png(arg[2],image)
