require 'lib.fst'

import_all(ocr)
import_all(nustring)

function transcript_to_fst(transcript)
    assert(transcript)
    local fst = ocr.make_StandardFst()
    -- tolua.takeownership(fst)
    local s = fst:newState()
    fst:setStart(s)
    for j = 0, transcript--[=[[i]]=]:length() - 1 do
        local c = transcript--[=[[i]]=]:at(j)
        if c:ord() == 32 then
            -- add a loop to the current state
            fst:addTransition(s, s, c:ord(), 0, c:ord())
        else
            -- add an arc to the next state
            local t = fst:newState()
            fst:addTransition(s, t, c:ord(), 0, c:ord())
            s = t
        end
    end
    fst:setAccept(s)
    return fst
end


--- Check if the table contains the given item.
function contains(table, item)
    for key, value in pairs(table) do
        if value == item then
            return true
        end
    end
    return false
end

--- Return a table of characters used in the given bunch of nustrings.
function get_alphabet(transcripts)
    local x = {}
    for i = 1, #transcripts do
        local t = transcripts[i]
        for j = 0, t:length() - 1 do
            local code = t:at(j):ord()
            if not contains(x, code) then
                table.insert(x, code)
            end
        end
    end
    return x
end

function test_editdist(s1, s2)
    -- print(s1, s2)
    t = {nustring(s1), nustring(s2)}
    fst1 = transcript_to_fst(t[1])
    fst2 = transcript_to_fst(t[2])
    fst_ed = fst.make_EditDistanceFst(get_alphabet(t))
    inputs = intarray()
    v1 = intarray()
    v2 = intarray()
    v3 = intarray()
    outputs = intarray()
    costs = floatarray()
    a_star_in_composition(inputs, v1, v2, v3, outputs, costs, fst1, fst_ed, fst2)
    --[[local n = outputs:length()
    for i = 0, n - 1 do
        print(string.char(inputs:at(i)), string.char(outputs:at(i)), costs:at(i))
    end
    print(narray.sum(costs))
    print(edit_distance(t[1], t[2]))]]
    assert(narray.sum(costs) == edit_distance(t[1], t[2]))
end

function random_string(letters, length)
    local s = ''
    for i = 1, length do
        s = s..string.char(letters:byte(math.random(#letters)))
    end
    return s
end

function test_editdist_on_random_string(letters, len)
    test_editdist(random_string(letters, len), random_string(letters, len))
end

for i = 1, 100 do
    test_editdist_on_random_string('ab', math.random(10))
    test_editdist_on_random_string('abc', math.random(10))
    test_editdist_on_random_string('abcd', math.random(10))
    test_editdist_on_random_string('abcde', math.random(10))
end
