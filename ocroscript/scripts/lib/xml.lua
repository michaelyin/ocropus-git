-- XML parsing

xml = {}

-- by Roberto Ierusalimschy
-- copied from http://lua-users.org/wiki/LuaXml
-- fixed a bit

local function xml_parseargs(s)
  local arg = {}
  string.gsub(s, "(%w+)%s*=%s*([\"'])(.-)%2", function (w, _, a)
    arg[w] = a
  end)
  return arg
end

--- Parse XML.
---
--- See hocr module for description of the format.
---
--- This function is based on code by Roberto Ierusalimschy
--- with changes to the output DOM format.
--- Original is at http://lua-users.org/wiki/LuaXml
---
--- @param s The input string containing XML data.
--- @return The DOM tree.
function xml.collect(s)
  local stack = {}
  local top = {}
  table.insert(stack, top)
  local ni,c,label,xarg, empty
  local i, j = 1, 1
  while true do
    ni,j,c,label,xarg, empty = string.find(s, "<(%/?)(%w+)(.-)(%/?)>", i)
    if not ni then break end
    local text = string.sub(s, i, ni-1)
    if not string.find(text, "^%s*$") then
      table.insert(top, text)
    end
    if empty == "/" then  -- empty element tag
      local node = xml_parseargs(xarg)
      node.tag = label
      table.insert(top, node)
    elseif c == "" then   -- start tag
      top = xml_parseargs(xarg)
      top.tag = label
      table.insert(stack, top)   -- new level
    else  -- end tag
      local toclose = table.remove(stack)  -- remove top
      top = stack[#stack]
      if #stack < 1 then
        error("nothing to close with "..label)
      end
      if toclose.tag ~= label then
        error("trying to close "..toclose.tag.." with "..label)
      end
      table.insert(top, toclose)
    end
    i = j+1
  end
  local text = string.sub(s, i)
  if not string.find(text, "^%s*$") then
    table.insert(stack[stack.n], text)
  end
  if #stack > 1 then
    error("unclosed "..stack[stack.n].label)
  end
  return stack[1]
end

--------------------------------------------------------------------------------

function xml.unescape_entities_to_nustring(text)
    --return text:gsub('&amp;','&'):gsub('&lt;','<'):gsub('&gt;','>')
    -- We now have a C++ implementation that does a better job.
    local n = nustring()
    ocr.xml_unescape(n, text)
    return n
end

function xml.unescape_entities(text)
    return xml.unescape_entities_to_nustring(text):utf8()
end

function xml.escape_entities(text)
    return text:gsub('&','&amp;'):gsub('<','&lt;'):gsub('>','&gt;')
end

function xml.get_cinfo_bboxes_by_line_DOM(line_DOM)
    local function build_list_of_bboxes(node, list)
        if node.class == 'ocr_cinfo' then
            props = hocr.parse_properties(node.title)
            if props.bbox then
                table.insert(list, props.bbox)
            end
        elseif type(node) == 'table' then
            for i = 1, #node do
                build_list_of_bboxes(node[i], list)
            end
        end
    end
    local list = {}
    build_list_of_bboxes(line_DOM, list)
    return list
end

function xml.get_raw_transcript_by_line_DOM(line_DOM)
    local function build_list_of_texts(node, list)
        if type(node) == 'string' then
            table.insert(list, node)
        elseif type(node) == 'table' then
            for i = 1, #node do
                build_list_of_texts(node[i], list)
            end
        end
    end
    local list = {}
    build_list_of_texts(line_DOM, list)
    if #list == 0 then return end
    local result = list[1]
    for i = 2, #list do
        result = result .. ' ' .. list[i]
    end
    return result
end

function xml.get_transcript_by_line_DOM(line_DOM)
    return xml.unescape_entities(xml.get_raw_transcript_by_line_DOM(line_DOM))
end

function xml.get_transcript_nustring_by_line_DOM(line_DOM)
    return xml.unescape_entities_to_nustring(
        xml.get_raw_transcript_by_line_DOM(line_DOM))
end

function xml.get_list_of_subitems_by_DOM(node, subitem_type)
    local function build_list_of_subitems(node, subitem_type, list)
        if type(node) == 'table' then
            if node.class == subitem_type then
                table.insert(list, node)
            end
            for i = 1, #node do
                build_list_of_subitems(node[i], subitem_type, list)
            end
        end
    end
    local result = {}
    build_list_of_subitems(node, subitem_type, result)
    return result
end


