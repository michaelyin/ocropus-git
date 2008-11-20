require 'lib.hocr'

if #arg ~= 1 then
    print("Usage: "..arg[0].." <input.txt>")
    os.exit(1)
end

body_DOM = {tag = 'body'}
page_DOM = {tag = 'div'}
--page.description = arg[0]
for line in io.lines(arg[1]) do
    if line ~= "" then
        table.insert(page_DOM, {tag='div',
                                class='ocr_line',
                                escape_for_HTML(nustring(line))})
    end
end
table.insert(body_DOM, page_DOM)

doc_DOM = get_html_tag()
table.insert(doc_DOM, get_head_tag())
table.insert(doc_DOM, '\n')
table.insert(doc_DOM, body_DOM)
dump_DOM(io.stdout, doc_DOM, html_preamble)
print()
