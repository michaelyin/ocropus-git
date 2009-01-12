-- just for logging the transcriptions
-- since sometimes we produce "garbage" transcription, it's necessary to avoid
-- special characters for the loggers
function tohtml(str)
    return ((str:gsub('&',"&amp;")):gsub('<',"&lt;")):gsub('>',"&gt;")
end

-- perform a binary search in a sorted table
-- return nil if the value is not found,
-- otherwise the position of the value in the sorted table
-- did not find a builtin function yet
function binsearch(t,value)
    local v_pivot,left,right,p_pivot = nil,1,#t,0
    while left <= right do
        p_pivot = math.floor((left+right)/2)
        v_pivot = t[p_pivot]
        if value == v_pivot then
            return p_pivot
        elseif value < v_pivot then
            right = p_pivot - 1
        else
            left = p_pivot + 1
        end
    end
    return nil
end

--extract the words of a line into a table
function words_in_line(w,l)
    for word in string.gfind(l,"%a+") do
        table.insert(w, word)
    end
end

--score a line with a strict matching according to a dictionary
function score_line(input,dict)
    if input:len() == 0 then    -- no text
        return 0
    elseif input:len() < 10 then
        return -2               -- too short to make a decision
    else
        local words = {}
        words_in_line(words,input:lower())  -- extract the words
        local c = 0.
        for i,v in ipairs(words) do         -- for all words
            if binsearch(dict,v) ~= nil then    -- try to find a candidate in dict
                c = c + v:len()+1           -- if yes, say it's good
            end
        end
        if c < 7 then           -- a small transcription is ambiguous
            return -3
        else
            return c/(input:len()+1)    -- give the score
        end
    end
end

--read a list of words and store in a table
function read_dict(t,path)
    file = util.secure_open(path, "r")  -- secure open
    for line in file:lines() do         -- for all words
        if line:len() > 1 then          -- that are words
            table.insert(t,line:lower())-- insert in lowercase
        end
    end
    table.sort(t)                       -- sort for a further binary search
    file:close()
end

--read the dictionary given with ocropus
--please check is it is the right path
function read_english_dict(t)
    read_dict(t,"../data/words/en-us")  -- only works if script called from ../ocroscript
end
