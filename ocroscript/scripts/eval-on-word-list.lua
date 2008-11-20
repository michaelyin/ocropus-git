require 'lib.util'
require 'lib.dataset'

if #arg ~= 3 then
    print "usage: oscroscript eval-on-word-list <dataset> <bpnetfile> <logfile>"
    --print "<bpnetfile> ../data/models/neural-net-file.nn"

    os.exit(1)
end

local c = make_BpnetClassifier()
local cc = make_AdaptClassifier(c)
cc:load(arg[2])

local seg = make_CurvedCutSegmenter()
local ocr = make_NewGroupingLineOCR(cc, seg, true) --if the bpnet contains line info, set to true

total = 0
errors = 0
curent_nb = 0
max_number = 100
log = io.open(arg[3],"w")
for line, gt in datasets.dataset_entries(arg[1]) do
    fst = make_StandardFst()
    ocr:recognizeLine(fst, line)
    s = nustring()
    fst:bestpath(s)
    total = total + 1
    if s:utf8() ~= gt then
        errors = errors + 1
        log:write(string.format("%s\t%s\t%s\n", total, s:utf8(), gt))
        print(string.format("%s\t%s\t%s", total, s:utf8(), gt))
        log:flush()
    end
    log:write(string.format("%d word errors among %d  (%g)\n", errors, total, (100.*errors)/total))
    print(string.format("%d word errors among %d  (%g)", errors, total, (100.*errors)/total))
    log:flush()
    curent_nb = curent_nb + 1
    if curent_nb >= max_number then
        break
    end
end
log:close()
