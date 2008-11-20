import_all(ocr)
require 'lib.util'
dofile("utest.lua")

text = "This is a test string"
test_assert(edit_distance(nustring(text),nustring(text),1,1,1) == 0,
        "Edit distance test failed")

text0 = "This is a test string"
test_assert(edit_distance(nustring(text),nustring(text0),1,1,1) == 0,
        "Edit distance test failed")

text1 = "This is  test string"
test_assert(edit_distance(nustring(text),nustring(text1),1,1,1) == 1,
        "Edit distance test failed")

text1 = "this is a test string"
test_assert(edit_distance(nustring(text),nustring(text1),1,1,1) == 1,
        "Edit distance test failed")

text1 = "This is a test string."
test_assert(edit_distance(nustring(text),nustring(text1),1,1,1) == 1,
        "Edit distance test failed")

text2 = "This is a tegst strin"
test_assert(edit_distance(nustring(text),nustring(text2),1,1,1) == 2,
        "Edit distance test failed")

text2 = "This is a test stringgg"
test_assert(edit_distance(nustring(text),nustring(text2),1,1,1) == 2,
        "Edit distance test failed")

text4 = "This is      test string"
test_assert(edit_distance(nustring(text),nustring(text4),1,1,1) == 4,
        "Edit distance test failed")

text5 = "is a test string"
test_assert(edit_distance(nustring(text),nustring(text5),1,1,1) == 5,
        "Edit distance test failed")


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
1) The rabbit-hole went straight on like a tunnel for some way, and then
2) dipped suddenly down, so suddenly that Alice had not a moment to think
3) about stopping herself before she found herself falling down a very deep well.
4) Either the well was very deep, or she fell very slowly, for she had plenty
5) of time as she went down to look about her and to wonder what was going]]

block_move_cost=0
test_assert(block_move_edit_distance(nustring(textlong),nustring(textlong),block_move_cost) == 0, 
        "Block Move Edit distance test failed")
block_move_cost=100
test_assert(block_move_edit_distance(nustring(textlong),nustring(textlong),block_move_cost) == 0, 
        "Block Move Edit distance test failed")

