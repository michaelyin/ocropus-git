fst = {}

--- Create an FST for edit distance computation.
function fst.make_EditDistanceFst(alphabet)
    local fst = pfst.make_StandardFst()
    local main = fst:newState()
    local aux1 = fst:newState()
    local aux2 = fst:newState()
    fst:setStart(main)
    fst:setAccept(main)
    fst:addTransition(main, aux1, 0, 0.5, 0)
    fst:addTransition(aux1, main, 0, 0.5, 0)
    fst:addTransition(main, aux2, 0, 0.5, 0)
    fst:addTransition(aux2, main, 0, 0.5, 0)
    for key, value in pairs(alphabet) do
        fst:addTransition(main, main, value, 0, value)
        fst:addTransition(main, aux1, 0, 0.5, value)
        fst:addTransition(aux1, main, value, 0.5, 0)
        fst:addTransition(aux2, main, 0, 0.5, value)
        fst:addTransition(main, aux2, value, 0.5, 0)
    end
    return fst
end

local function contains(table, item)
    for k, v in pairs(table) do
        if v == item then
            return true
        end
    end
    return false
end

--- Get the set of letters used in the transcript as a single string
function fst.alphabet(s)
    if type(s) == 'string' then
        s = nustring(s)
    end
    
    local result = {}
    for i = 0, s:length() - 1 do
        local c = s:at(i):ord()
        if not contains(result, c) then
            table.insert(result, c)
        end
    end
    return result
end

function fst.union(t, ...)
    if type(t) ~= 'table' then
        t = {t, ...}
    end

    local f = pfst.make_StandardFst()
    local start = f:newState()
    f:setStart(start)

    for k, v in pairs(t) do
        if type(v) == 'string' or tolua.type(v) == 'nustring' then
            v = fst.line(v)
        end
        ocr.fst_insert(f, v, start)
    end

    return f
end

function fst.concat(t, ...)
    if type(t) ~= 'table' then
        t = {t, ...}
    end

    local fst = pfst.make_StandardFst()
    
    fst_copy(fst, t[#t])
    for i = #t - 1, 1, -1 do
        local new_start = fst:newState()
        local old_start = fst:getStart()
        fst:setStart(new_start)
        fst_insert(fst, t[i], new_start, old_start)
    end
    return fst
end

function fst.line(string)
    if type(string) == 'string' then
        string = nustring(string)
    end
    
    local fst = pfst.make_StandardFst()
    local s = fst:newState()
    fst:setStart(s)
    for i = 0, string:length() - 1 do
        local c = string:at(i):ord()
        local t = fst:newState()
        fst:addTransition(s, t, c, 0, c)
        s = t
    end
    fst:setAccept(s)
    return fst
end
