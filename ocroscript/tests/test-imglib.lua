function test_label_components_Wx1()
    for w = 1, 5 do
        a = intarray(w, 1)
        narray.fill(a, 0)
        iulib.label_components(a)
        narray.fill(a, 1)
        iulib.label_components(a)
    end
end

if not pcall(test_label_components_Wx1) then
    print "please update iulib to revision 62"
end
