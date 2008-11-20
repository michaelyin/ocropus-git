require 'lib.util'
require 'lib.editdist'

if #arg ~= 2 and #arg ~= 3 then
    print("Usage: ocroscript "..arg[0].." file1 file2 [block_movement_cost]")
    os.exit(1)
end

text1 = io.open(arg[1]):read("*all"):gsub("[^a-zA-Z0-9]","")
text2 = io.open(arg[2]):read("*all"):gsub("[^a-zA-Z0-9]","")

if arg[3] then
    c = arg[3] + 0
else
    c = 2
end

print(editdist.roundtrip(nustring(text1), nustring(text2), c))
