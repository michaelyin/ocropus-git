import_all(ocr)
import_all(graphics)

-- dinit(1000,1000)

dofile("utest.lua")

txtimgseg = make_TextImageSegByLeptonica()

function outimage(file)
    -- note: order matches for these patterns
    if string.find(file,"S001BIN") then return "cleanup-images/S001BIN-text.png" end
    if string.find(file,"S00BBIN") then return "cleanup-images/S00BBIN-text.png" end
    if string.find(file,"S03LBIN") then return "cleanup-images/S03LBIN-text.png" end
    return "-1"
end 

images = {
"cleanup-images/S001BIN-clean.png",
"cleanup-images/S00BBIN-clean.png",
"cleanup-images/S03LBIN-clean.png",
}

logger = Logger("foo")
function try_txtimgseg(file)
    print(string.format("Testing image: %s",file))
    image = bytearray:new()
    tolua.takeownership(image)
    iulib.read_image_binary(image,file)
         
    note("Original result")
    result = outimage(file)
    resultimage = bytearray()
    iulib.read_image_binary(resultimage,result)
    dshow(resultimage,"a")

    note("Obtained result")
    probabilities = intarray()
    narray.fill(probabilities, 0)
    txtimgseg:textImageProbabilities(probabilities,image)
    dshow(probabilities,"b")
    dwait()

    seg = bytearray()
    narray.copy(seg, probabilities)
    logger:log("seg", seg)
    logger:log("resultimage", resultimage)
    test_assert(narray.equal(seg,resultimage),"result differs from original")
end

for i,file in ipairs(images) do
    try_txtimgseg(file)
    collectgarbage()
end
