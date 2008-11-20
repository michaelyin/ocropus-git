-- Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz 
-- or its licensors, as applicable.
-- 
-- You may not use this file except under the terms of the accompanying license.
-- 
-- Licensed under the Apache License, Version 2.0 (the "License"); you
-- may not use this file except in compliance with the License. You may
-- obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
-- 
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
-- 
-- Project: ocroscript
-- File: recognize.lua
-- Purpose: top-level recognition script
-- Responsible: mezhirov
-- Reviewer: 
-- Primary Repository: 
-- Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

import_all(ocr)
import_all(graphics)
import_all(iulib)

segmenter = make_SegmentPageByRAST()
page_gray = bytearray()
page_binary = bytearray()
page_seg = intarray()

page = 0

for i = 1, #arg do
   pages = Pages()
   pages:parseSpec(arg[i])
   while pages:nextPage() do
      page = page+1
      line = 0
      pages:getBinary(page_binary)
      pages:getGray(page_gray)
      segmenter:segment(page_seg,page_binary)
      local extractor = RegionExtractor()
      extractor:setPageLines(page_seg)
      local line_gray = bytearray()
      for i = 1, extractor:length() - 1 do
	 line = line+1
	 extractor:extract(line_gray, page_gray, i)
	 bbox = extractor:bbox(i)
	 file = string.format("p%04d_l%04d.png",page,line)
	 print(file)
	 write_image_gray(file,line_gray)
      end
   end
end
