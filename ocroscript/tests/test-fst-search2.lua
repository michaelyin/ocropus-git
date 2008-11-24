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

function compare_openfst_search2_with_custom_search(fst1, fst2, callback)
    local fst_builder1 = openfst.make_FstBuilder()
    local fst_builder2 = openfst.make_FstBuilder()
    pfst.fst_copy(fst_builder1, fst1)
    pfst.fst_copy(fst_builder2, fst2)
    local openfst_fst1 = fst_builder1:take()
    local openfst_fst2 = fst_builder2:take()

    -- OpenFST's search
    local openfst_outputs = nustring()
    local openfst_costs = floatarray()
    local openfst_inputs = intarray()
    local openfst_cost = openfst.bestpath2(openfst_outputs, openfst_costs,
                          openfst_inputs, openfst_fst1, openfst_fst2, true --[[copy_eps]])

    -- Other search
    local custom_outputs = intarray()
    local custom_costs = floatarray()
    local custom_vertices1 = intarray()
    local custom_vertices2 = intarray()
    local custom_inputs = intarray()
    callback(custom_inputs, custom_vertices1, custom_vertices2,
             custom_outputs, custom_costs, fst1, fst2)
    
    --[[print('openfst', openfst_cost)
    print('our', narray.sum(custom_costs))
    
    print("openfst output's inputs:")
    print_array(openfst_inputs)
    print("our output's inputs:")
    print_array(custom_inputs)]]

    -- if the FST is disconnected, we can't really check anything meaningful
    if narray.sum(custom_costs) > 1e37 then
        assert(openfst_cost > 1e37)
        return
    end

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
    assert(narray.equal(custom_costs, openfst_costs))
    if custom_vertices1:length() > 0 and custom_vertices2:length() > 0 then
        local last_vertex1 = custom_vertices1:at(custom_vertices1:length() - 1)
        local last_vertex2 = custom_vertices2:at(custom_vertices2:length() - 1)
        assert(custom_costs_top == fst1:getAcceptCost(last_vertex1) + fst2:getAcceptCost(last_vertex2))
    end

    -- too bad we can't compare vertices because openfst doesn't return them
    -- (or maybe it's our openfst binding)
    
    -- compare outputs
    assert(custom_outputs:length() == openfst_outputs:length() + 1)
    local custom_outputs_top = custom_outputs:pop()
    assert(custom_outputs_top == 0)
    local n = openfst_outputs:length()
    for i = 0, n - 1 do
        assert(openfst_outputs:at(i):ord() == custom_outputs:at(i))
    end
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
            f:addTransition(i, i + j, math.random(32, 33),
                                      math.random(),
                                      math.random(32, 33))
        end
    end
    return f
end

for i = 1, 100 do
    fst1 = generate_random_fst(20, 10)
    fst2 = generate_random_fst(10, 5)
    fst1:save('1.fst')
    fst2:save('2.fst')
    --[[compare_openfst_search2_with_custom_search(fst1, fst2, 
        function(i, v1, v2, o, c, fst1, fst2)
            ocr.beam_search_in_composition(i, v1, v2, o, c, fst1, fst2, 100)
        end
    ) --not testing beam search for now ]]
    compare_openfst_search2_with_custom_search(fst1, fst2,
        function(i, v1, v2, o, c, fst1, fst2)
            pfst.a_star_in_composition(i, v1, v2, o, c, fst1, fst2)
        end
    )
end
