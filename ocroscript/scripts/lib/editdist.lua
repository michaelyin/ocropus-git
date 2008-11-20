-- This file continues C++ editdist code.
-- Here the "roundtrip" edit distance is implemented (see below).

editdist = {}

local function edit_cost_with_garbage(from, to, c)
    local jumps_from = intarray()
    local jumps_to = intarray()
    local cost = ocr.edit_cost_for_layout(jumps_from, jumps_to, from, to, c)
    local covered = bytearray()
    ocr.analyze_jumps(covered, jumps_from, jumps_to, from:length())
    local skipped = nustring.nustring()
    local nonskipped = nustring.nustring()
    for i = 0, from:length() - 1 do
        if covered:at(i) ~= 0 then
            nonskipped:push(from:at(i))
        else
            skipped:push(from:at(i))
        end
    end
    return nonskipped, skipped, cost, jumps_from:length()
end

local function count_newlines(text)
    local result = 0
    for i = 0, text:length() - 1 do
        if text:at(i):ord() == string.byte('\n') then
            result = result + 1
        end
    end
    return result
end

local function max(a, b)
    if a > b then
        return a
    else
        return b
    end
end

local function min(a, b)
    if a < b then
        return a
    else
        return b
    end
end


--- Compute "roundtrip" edit distance between 2 nustrings.
--- `c' is the cost of block movement (aka cursor jump).
--- \return 4 values:
---      -  number of jumps (approximates number of layout analysis errors);
---      -  number of lines present in `a' but not in `b';
---      -  number of lines present in `b' but not in `a';
---      -  edit distance between common lines in `a' and `b' without jump costs
---             (approximates number of OCR errors)
function editdist.roundtrip(a, b, c)
    local a_nonskipped, a_skipped, a_cost = edit_cost_with_garbage(a, b, c)
    local b_nonskipped, b_skipped, b_cost = edit_cost_with_garbage(b, a, c)
    local a_all, a_bad, a_cost2, aj = edit_cost_with_garbage(a, b_nonskipped, c)
    local b_all, b_bad, b_cost2, bj = edit_cost_with_garbage(b, a_nonskipped, c)
    
    local a_all3, a_bad3, a_cost3, aj3 = edit_cost_with_garbage(a_nonskipped,
                                                                b_nonskipped, c)
    local a_all3, b_bad3, b_cost3, bj3 = edit_cost_with_garbage(b_nonskipped,
                                                                a_nonskipped, c)
    return aj + bj - min(aj3, bj3),
           count_newlines(a_skipped), 
           count_newlines(b_skipped),
           max(a_cost3 - aj3 * c, b_cost3 - bj3 * c)
end
