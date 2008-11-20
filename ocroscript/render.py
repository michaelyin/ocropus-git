#! /usr/bin/python

# Copyright 2006-2008 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
# or its licensors, as applicable.
#
# You may not use this file except under the terms of the accompanying license.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You may
# obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Project: ocroscript
# File: render.py
# Purpose: create a database of synthetic text with the PIL library
# Responsible: mezhirov
# Reviewer: rangoni
# Primary Repository:
# Web Sites: www.iupr.org, www.dfki.de, www.ocropus.org

from PIL import Image, ImageFont, ImageDraw
from numpy import *
from scipy import ndimage
import os, sys
from pylab import *
from random import randrange
import codecs

#the script reads the text file in argv[1] (saved in utf8 format)
#for each line in the text file, an image of the line and a transcription
#file are written in folder specified by argv[3].
#argv[3] is a prefix (folder+'/'+name), a subfolder name+'.files'
#will be created to store the images and transcriptions.
#a list file (name) will be created to store the location of
#images and transcriptions
#the font is specified by argv[1], eg: /usr/share/fonts/truetype/freefont/FreeSerif.ttf

def render(text, font, w=None, h=130):
    if not w:
        w = h * len(text)
    image = Image.new("L", (w,h), 255)
    draw = ImageDraw.Draw(image)
    # without that space before the text, some letters look cut a little
    draw.text((0, 0), ' ' + text, font=font)
    del draw
    return double(asarray(image))

if len(sys.argv) != 4:
    print "Usage: render.py <text-file> <font.ttf> <output_prefix>"
    print "ex: render.py mytext.txt /usr/share/fonts/truetype/freefont/FreeSerif.ttf testrenderpy/img"
    exit(1)

text = codecs.open(sys.argv[1],'r','utf8')

font_name = sys.argv[2]
prefix = sys.argv[3] + ".files"
list = open(sys.argv[3], "w")
font = ImageFont.truetype(font_name, 100)
if not os.path.isdir(prefix):
    os.mkdir(prefix)
    
i = 0
for line in text:
    current = line.rstrip()
    img_line = render(current, font)
    img_line = ndimage.zoom(img_line, .3)
    min_img_line = img_line.min()
    max_img_line = img_line.max()
    img_line = 255 * (img_line - min_img_line) / (max_img_line - min_img_line)
    filename = "%s/%05d" % (prefix, i)
    Image.fromarray(img_line.astype('uint8'), 'L').save(filename + '.png')
    transcript = codecs.open(filename + '.txt', 'w', 'utf-8')
    transcript.write(line)
    transcript.close()
    list.write(filename + '.png ' + filename + '.txt\n')
    i += 1
list.close()
