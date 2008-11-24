if not openfst then
    print "OpenFST is disabled, we can't test it."
    os.exit(0)
end


function print_array(a)
    for i = 0, a:length() - 1 do
        print(a:at(i))
    end
    print()
end

function generate_random_fst(nstates, maxspan)
    local f = pfst.make_StandardFst()
    for i = 0, nstates - 1 do
        local s = f:newState()
        assert(s == i)
    end
    f:setStart(0)
    f:setAccept(nstates - 1)
    for i = 0, nstates - 1 do
        for j = 1, maxspan do
            if i + j >= nstates then
                break
            end
            f:addTransition(i, i + j, math.random(32, 128),
                                      math.random(),
                                      math.random(32, 128))
        end
    end
    return f
end

function test_compare_openfst_search_with_custom_search(fst, callback)
    local fst_builder = openfst.make_FstBuilder()
    pfst.fst_copy(fst_builder, fst)
    local openfst_fst = fst_builder:take()

    -- OpenFST's search
    local openfst_outputs = nustring()
    local openfst_costs = floatarray()
    local openfst_inputs = intarray()
    openfst.bestpath(openfst_outputs, openfst_costs, openfst_inputs,
                     openfst_fst, false --[[that's copy_eps; what's that?]])

    -- Other search
    local custom_outputs = intarray()
    local custom_costs = floatarray()
    local custom_vertices = intarray()
    local custom_inputs = intarray()
    callback(custom_inputs, custom_vertices, custom_outputs, custom_costs, fst)

    --openfst_fst:Write("_debug.fst")

    -- compare inputs
    -- our search has that extra 0 padding
    assert(custom_inputs:length() == openfst_inputs:length() + 1)
    local custom_inputs_top = custom_inputs:pop()
    assert(custom_inputs_top == 0)
    assert(narray.equal(custom_inputs, openfst_inputs))

    -- compare costs
    assert(custom_costs:length() == openfst_costs:length() + 1)
    -- our search returns also the accept cost of the last vertex
    local custom_costs_top = custom_costs:pop()
    local last_vertex = custom_vertices:at(custom_vertices:length() - 1)
    assert(custom_costs_top == f:getAcceptCost(last_vertex))
    assert(narray.equal(custom_costs, openfst_costs))

    -- too bad we can't compare vertices because openfst doesn't return them
    -- (or maybe it's our openfst binding)
    
    -- compare outputs
    assert(custom_outputs:length() == openfst_outputs:length() + 1)
    local custom_outputs_top = custom_outputs:pop()
    assert(custom_outputs_top == 0)
    -- we have a little problem here because openfst_outputs is a nustring,
    -- and our outputs is a intarray. Well, we have to make a Lua loop, then.
    local n = openfst_outputs:length()
    for i = 0, n - 1 do
        assert(openfst_outputs:at(i):ord() == custom_outputs:at(i))
    end
end

--f = make_StandardFst()
--test_compare_openfst_search_with_beam_search(f)
for i = 1, 1000 do
    f = generate_random_fst(math.random(20,30), math.random(1, 6))
    --[[test_compare_openfst_search_with_custom_search(f, function(i, v, o, c, fst)
        ocr.beam_search(i, v, o, c, fst, 21)
    end) not testing beam search - it's currently private ]]
    test_compare_openfst_search_with_custom_search(f, function(i, v, o, c, fst)
        pfst.a_star(i, v, o, c, fst)
    end)
end
