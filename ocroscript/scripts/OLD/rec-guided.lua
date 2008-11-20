require 'lib.util'

coef = .05

function all_items_are_equal(list)
    for i = 2, #list do
        if list[1] ~= list[i] then
            return false
        end
    end
    return true
end

if #arg < 2 then
    print('Usage: ocroscript '..arg[0]..' <line.png> <variants.txt> [coef]')
    print('This script will recognize the line guided by variants proposed')
    os.exit(1)
end

image = read_image_gray_checked(arg[1]);

variants = {}
for i in io.lines(arg[2]) do
    table.insert(variants, i)
end

if #variants == 0 then
    print 'error: no variants'
    os.exit(1)
end

if all_items_are_equal(variants) then
    print(variants[1])
    os.exit(0)
end

local bpfile = os.getenv("bpnet")
if not bpfile then
   bpfile = ocrodata.."models/neural-net-file.nn"
   stream = io.open(bpfile)
   if not stream then bpfile = "../data/models/neural-net-file.nn" 
   else stream:close() end
end
local bpnet = make_NewBpnetLineOCR(bpfile)

fst = make_StandardFst()
bpnet:recognizeLine(fst, image)

if arg[3] then
    coef = arg[3] + 0
end
for i = 1, #variants do
    beam_search_and_rescore(fst, transcript_to_fst(nustring(variants[i])), coef)
end

result = nustring()
fst:bestpath(result)
print(result:utf8())
