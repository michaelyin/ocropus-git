require 'lib.datasets'

import_all(ocr)
                                                               
function create_model(fst)
    fst:clear()
    local start = fst:newState()
    local b = fst:newState()
    local c = fst:newState()
    fst:setStart(start)
    fst:setAccept(b)
    fst:setAccept(c)
    fst:addTransition(start, b, string.byte('b'), 0, string.byte('a'))
    fst:addTransition(start, c, string.byte('c'), 0, string.byte('a'))
end

fst1 = datasets.transcript_to_fst(nustring('a'))
fst2 = pfst.make_StandardTrainableFst()
create_model(fst2)
fst3b = datasets.transcript_to_fst(nustring('b'))
fst3c = datasets.transcript_to_fst(nustring('c'))

function get_weights_from_start(fst)
    local inputs = intarray()
    local targets = intarray()
    local outputs = intarray()
    local costs = floatarray()
    fst:arcs(inputs, targets, outputs, costs, fst:getStart())
    local n = targets:length()
    local result = {}
    for i = 0, n - 1 do
        result[string.char(outputs:at(i))] = costs:at(i)
    end
    return result
end


for i = 1, 10 do
    fst2:expectation(fst1, fst3b)
end
fst2:maximization()

assert(get_weights_from_start(fst2)['b'] > 0.9)
assert(get_weights_from_start(fst2)['c'] < 0.1)

for i = 1, 10 do
    fst2:expectation(fst1, fst3c)
end
fst2:maximization()

assert(get_weights_from_start(fst2)['b'] < 0.1)
assert(get_weights_from_start(fst2)['c'] > 0.9)
