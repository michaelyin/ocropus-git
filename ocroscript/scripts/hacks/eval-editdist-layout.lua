require 'lib.util'
require 'lib.editdist'

if #arg ~= 1 then
    print("Usage: ocroscript "..arg[0].." block_movement_cost_layout")
    os.exit(1)
end

block_move_cost_layout=arg[1]

textlong=[[Alice was beginning to get very tired of sitting by her sister on the bank,
and of having nothing to do: once or twice she had peeped into the book her
sister was reading, but it had no pictures or conversations in it, and what is
the use of a book, thought Alice without pictures or conversation?
So she was considering in her own mind (as well as she could, for the hot
day made her feel very sleepy and stupid), whether the pleasure of making a
daisychain would be worth the trouble of getting up and picking the daisies,
when suddenly a White Rabbit with pink eyes ran close by her.
There was nothing so VERY remarkable in that; nor did Alice think it so
VERY much out of the way to hear the Rabbit say to itself, Oh dear! Oh
dear! I shall be late! (when she thought it over afterwards, it occurred to
her that she ought to have wondered at this, but at the time it all seemed
quite natural); but when the Rabbit actually TOOK A WATCH OUT OF
ITS WAISTCOAT POCKET, and looked at it, and then hurried on, Alice
started to her feet, for it flashed across her mind that she had never before
seen a rabbit with either a waistcoat-pocket, or a watch to take out of it, and
burning with curiosity, she ran across the field after it, and fortunately was
just in time to see it pop down a large rabbit-hole under the hedge.
(1) The rabbit-hole went straight on like a tunnel for some way, and then
(2) dipped suddenly down, so suddenly that Alice had not a moment to think
(3) about stopping herself before she found herself falling down a very deep well.
(4) Either the well was very deep, or she fell very slowly, for she had plenty
(5) of time as she went down to look about her and to wonder what was going]]

print("=========== horizontally merged text with no OCR errors ===========")
textlong_merged=[[Alice was beginning to get very tired of sitting by her sister on the bank, There was nothing so VERY remarkable in that; nor did Alice think it so
and of having nothing to do: once or twice she had peeped into the book her VERY much out of the way to hear the Rabbit say to itself, Oh dear! Oh
sister was reading, but it had no pictures or conversations in it, and what is dear! I shall be late! (when she thought it over afterwards, it occurred to
the use of a book, thought Alice without pictures or conversation? her that she ought to have wondered at this, but at the time it all seemed
So she was considering in her own mind (as well as she could, for the hot quite natural); but when the Rabbit actually TOOK A WATCH OUT OF
day made her feel very sleepy and stupid), whether the pleasure of making a ITS WAISTCOAT POCKET, and looked at it, and then hurried on, Alice
daisychain would be worth the trouble of getting up and picking the daisies, started to her feet, for it flashed across her mind that she had never before
when suddenly a White Rabbit with pink eyes ran close by her. seen a rabbit with either a waistcoat-pocket, or a watch to take out of it, and
burning with curiosity, she ran across the field after it, and fortunately was
just in time to see it pop down a large rabbit-hole under the hedge.
(1) The rabbit-hole went straight on like a tunnel for some way, and then
(2) dipped suddenly down, so suddenly that Alice had not a moment to think
(3) about stopping herself before she found herself falling down a very deep well.
(4) Either the well was very deep, or she fell very slowly, for she had plenty
(5) of time as she went down to look about her and to wonder what was going]]

block_move_cost=0
print("Edit distance with 0 block movement cost = " .. block_move_edit_distance(nustring(textlong),nustring(textlong_merged),block_move_cost))
block_move_cost=100
print("Edit distance with 100 block movement cost = " .. block_move_edit_distance(nustring(textlong),nustring(textlong_merged),block_move_cost))

jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 16")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)


print("=========== horizontally merged text with OCR errors ===========")
textlong_merged=[[Alice was beginning t0 get very tired 0f sitting by her sister 0n the bank, There was n0thing s0 VERY remarkable in that; n0r did Alice think it s0
and 0f having n0thing t0 d0: 0nce 0r twice she had peeped int0 the b00k her VERY much 0ut 0f the way t0 hear the Rabbit say t0 itself, 0h dear! 0h
sister was reading, but it had n0 pictures 0r c0nversati0ns in it, and what is dear! I shall be late! (when she th0ught it 0ver afterwards, it 0ccurred t0
the use 0f a b00k, th0ught Alice with0ut pictures 0r c0nversati0n? her that she 0ught t0 have w0ndered at this, but at the time it all seemed
S0 she was c0nsidering in her 0wn mind (as well as she c0uld, f0r the h0t quite natural); but when the Rabbit actually T00K A WATCH 0UT 0F
day made her feel very sleepy and stupid), whether the pleasure 0f making a ITS WAISTC0AT P0CKET, and l00ked at it, and then hurried 0n, Alice
daisychain w0uld be w0rth the tr0uble 0f getting up and picking the daisies, started t0 her feet, f0r it flashed acr0ss her mind that she had never bef0re
when suddenly a White Rabbit with pink eyes ran cl0se by her. seen a rabbit with either a waistc0at-p0cket, 0r a watch t0 take 0ut 0f it, and
burning with curi0sity, she ran acr0ss the field after it, and f0rtunately was
just in time t0 see it p0p d0wn a large rabbit-h0le under the hedge.
(1) The rabbit-hole went straight on like a tunnel for some way, and then
(2) dipped suddenly down, so suddenly that Alice had not a moment to think
(3) about stopping herself before she found herself falling down a very deep well.
(4) Either the well was very deep, or she fell very slowly, for she had plenty
(5) of time as she went down to look about her and to wonder what was going]]

jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 16")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)

print("=========== horizontally merged text with OCR errors and split text-line errors ===========")

textlong_merged=[[Alice was beginning t0 get very tired 0f sitting by her sister 0n the bank, There was n0thing s0 VERY remarkable in that; n0r did Alice think it s0
and 0f having n0thing t0 d0: 0nce 0r twice she had peeped int0 the b00k her VERY much 0ut 0f the way t0 hear the Rabbit say t0 itself, 0h dear! 0h
sister was reading, but it had n0 pictures 0r c0nversati0ns in it, and what is dear! I shall be late! (when she th0ught it 0ver afterwards, it 0ccurred t0
the use 0f a b00k, th0ught Alice with0ut pictures 0r c0nversati0n? her that she 0ught t0 have w0ndered at this, but at the time it all seemed
S0 she was c0nsidering in her 0wn mind (as well as she c0uld, f0r the h0t quite natural); but when the Rabbit actually T00K A WATCH 0UT 0F
day made her feel very sleepy and stupid), whether the pleasure 0f making a ITS WAISTC0AT P0CKET, and l00ked at it, and then hurried 0n, Alice
daisychain w0uld be w0rth the tr0uble 0f getting up and picking the daisies, started t0 her feet, f0r it flashed acr0ss her mind that she had never bef0re
when suddenly a White Rabbit with pink eyes ran cl0se by her. seen a rabbit with either a waistc0at-p0cket, 0r a watch t0 take 0ut 0f it, and
burning with curi0sity, she ran acr0ss the field after it, and f0rtunately was
just in time t0 see it p0p d0wn a large rabbit-h0le under the hedge.
(1)
(2)
(3)
(4)
(5) 
The rabbit-hole went straight on like a tunnel for some way, and then
dipped suddenly down, so suddenly that Alice had not a moment to think
about stopping herself before she found herself falling down a very deep well.
Either the well was very deep, or she fell very slowly, for she had plenty
of time as she went down to look about her and to wonder what was going]]


jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 21")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)


print("=========== horizontally merged text with OCR errors and partially detected text-line errors ===========")

textlong_merged=[[Alice was beginning t0 get very tired 0f sitting by her sister 0n the bank, There was n0thing s0 VERY remarkable in that; n0r did Alice think it s0
and 0f having n0thing t0 d0: 0nce 0r twice she had peeped int0 the b00k her VERY much 0ut 0f the way t0 hear the Rabbit say t0 itself, 0h dear! 0h
sister was reading, but it had n0 pictures 0r c0nversati0ns in it, and what is dear! I shall be late! (when she th0ught it 0ver afterwards, it 0ccurred t0
the use 0f a b00k, th0ught Alice with0ut pictures 0r c0nversati0n? her that she 0ught t0 have w0ndered at this, but at the time it all seemed
S0 she was c0nsidering in her 0wn mind (as well as she c0uld, f0r the h0t quite natural); but when the Rabbit actually T00K A WATCH 0UT 0F
day made her feel very sleepy and stupid), whether the pleasure 0f making a ITS WAISTC0AT P0CKET, and l00ked at it, and then hurried 0n, Alice
daisychain w0uld be w0rth the tr0uble 0f getting up and picking the daisies, started t0 her feet, f0r it flashed acr0ss her mind that she had never bef0re
when suddenly a White Rabbit with pink eyes ran cl0se by her. seen a rabbit with either a waistc0at-p0cket, 0r a watch t0 take 0ut 0f it, and
burning with curi0sity, she ran acr0ss the field after it, and f0rtunately was
just in time t0 see it p0p d0wn a large rabbit-h0le under the hedge.
The rabbit-hole went straight on like a tunnel for some way, and then
dipped suddenly down, so suddenly that Alice had not a moment to think
about stopping herself before she found herself falling down a very deep well.
Either the well was very deep, or she fell very slowly, for she had plenty
of time as she went down to look about her and to wonder what was going]]

jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 21")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)


print("=========== horizontally merged text with OCR errors and missed text-line errors ===========")

textlong_merged=[[Alice was beginning t0 get very tired 0f sitting by her sister 0n the bank, There was n0thing s0 VERY remarkable in that; n0r did Alice think it s0
and 0f having n0thing t0 d0: 0nce 0r twice she had peeped int0 the b00k her VERY much 0ut 0f the way t0 hear the Rabbit say t0 itself, 0h dear! 0h
sister was reading, but it had n0 pictures 0r c0nversati0ns in it, and what is dear! I shall be late! (when she th0ught it 0ver afterwards, it 0ccurred t0
the use 0f a b00k, th0ught Alice with0ut pictures 0r c0nversati0n? her that she 0ught t0 have w0ndered at this, but at the time it all seemed
S0 she was c0nsidering in her 0wn mind (as well as she c0uld, f0r the h0t quite natural); but when the Rabbit actually T00K A WATCH 0UT 0F
day made her feel very sleepy and stupid), whether the pleasure 0f making a ITS WAISTC0AT P0CKET, and l00ked at it, and then hurried 0n, Alice
daisychain w0uld be w0rth the tr0uble 0f getting up and picking the daisies, started t0 her feet, f0r it flashed acr0ss her mind that she had never bef0re
when suddenly a White Rabbit with pink eyes ran cl0se by her. seen a rabbit with either a waistc0at-p0cket, 0r a watch t0 take 0ut 0f it, and
burning with curi0sity, she ran acr0ss the field after it, and f0rtunately was
just in time t0 see it p0p d0wn a large rabbit-h0le under the hedge.
(2) dipped suddenly down, so suddenly that Alice had not a moment to think
(3) about stopping herself before she found herself falling down a very deep well.
(4) Either the well was very deep, or she fell very slowly, for she had plenty
(5) of time as she went down to look about her and to wonder what was going]]

jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 17")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)


print("=========== horizontally merged text with OCR errors and garbage text-line errors ===========")

textlong_merged=[[Alice was beginning t0 get very tired 0f sitting by her sister 0n the bank, There was n0thing s0 VERY remarkable in that; n0r did Alice think it s0
and 0f having n0thing t0 d0: 0nce 0r twice she had peeped int0 the b00k her VERY much 0ut 0f the way t0 hear the Rabbit say t0 itself, 0h dear! 0h
sister was reading, but it had n0 pictures 0r c0nversati0ns in it, and what is dear! I shall be late! (when she th0ught it 0ver afterwards, it 0ccurred t0
the use 0f a b00k, th0ught Alice with0ut pictures 0r c0nversati0n? her that she 0ught t0 have w0ndered at this, but at the time it all seemed
S0 she was c0nsidering in her 0wn mind (as well as she c0uld, f0r the h0t quite natural); but when the Rabbit actually T00K A WATCH 0UT 0F
day made her feel very sleepy and stupid), whether the pleasure 0f making a ITS WAISTC0AT P0CKET, and l00ked at it, and then hurried 0n, Alice
daisychain w0uld be w0rth the tr0uble 0f getting up and picking the daisies, started t0 her feet, f0r it flashed acr0ss her mind that she had never bef0re
when suddenly a White Rabbit with pink eyes ran cl0se by her. seen a rabbit with either a waistc0at-p0cket, 0r a watch t0 take 0ut 0f it, and
burning with curi0sity, she ran acr0ss the field after it, and f0rtunately was
just in time t0 see it p0p d0wn a large rabbit-h0le under the hedge.
HLDKABAHGSUPDHjksffngsbgs
sdfg sfyegsfg dfgwsgf hsfg
sfgsfg egsdfgdfherwrt jhj
jkyiogjk gk
loyihujwroekhh
setjstjioejorhoerh;jihoipbetbj
rhoithsjpstoihstpuih
(1) The rabbit-hole went straight on like a tunnel for some way, and then
(2) dipped suddenly down, so suddenly that Alice had not a moment to think
(3) about stopping herself before she found herself falling down a very deep well.
(4) Either the well was very deep, or she fell very slowly, for she had plenty
(5) of time as she went down to look about her and to wonder what was going]]

jumps, miss1, miss2, cost = editdist.roundtrip(nustring(textlong),nustring(textlong_merged),block_move_cost_layout)
print("Edit distance with "..block_move_cost_layout.." block movement cost = " .. cost)
print("Ground-Truth no of layout analysis errors = 16")
print("No of jumps = " .. jumps)
print("No of missed lines: ", miss1, miss2)

--print("Jumps from :")
--for i=0,jumps_from:length()-1 do
--    print(jumps_from:at(i))
--    print(string.sub(textlong,jumps_from:at(i)+1,jumps_from:at(i)+20))
--    print(jumps_to:at(i))
--    print(string.sub(textlong_merged,jumps_to:at(i)+1,jumps_to:at(i)+20))
--end
-- print("Edit distance tests succeeded")


