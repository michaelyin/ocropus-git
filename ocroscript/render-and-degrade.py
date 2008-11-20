#! /usr/bin/python

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
# File: render-and-degrade.py
# Purpose: create a database of degraded synthetic text with the PIL library
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
from numpy.random import random_sample, standard_normal

#the script is similar to render.py,
#it read the text file in argv[1] (saved in ut8 format)
#for argv[3] lines randomly picked in the text file,
#a degraded image of each line and a transcription
#file are written in folder specified by argv[4].
#argv[4] is a prefix (folder+'/'+name), a subfolder name+'.files'
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

def elastic_transform_map(shape, alpha=6, sigma=4):
    a = alpha * (2 * random_sample(shape) - 1)
    return ndimage.gaussian_filter(a, sigma)

def elastic_transform(image, alpha=6, sigma=4):
    d0 = elastic_transform_map(image.shape, alpha, sigma)
    d1 = elastic_transform_map(image.shape, alpha, sigma)
    def coord_map(coord):
        return coord[0] + d0[coord[0],coord[1]], \
               coord[1] + d1[coord[0],coord[1]]
    return ndimage.geometric_transform(image, coord_map)

def jitter(image, mean, sigma):
    d0 = standard_normal(image.shape) * sigma + mean
    d1 = standard_normal(image.shape) * sigma + mean
    def coord_map(coord):
        return coord[0] + d0[coord[0],coord[1]], \
               coord[1] + d1[coord[0],coord[1]]
    return ndimage.geometric_transform(image, coord_map)

def adjust_sensitivity(image, mean, sigma):
    return image - 255 * (standard_normal(image.shape) * sigma + mean)
    
def threshold(image, mean, sigma):
    t = 255 - 255 * (standard_normal(image.shape) * sigma + mean)
    return where(image < t, 0, 255)
    
def degrade(image, elastic_alpha=6, elastic_sigma=4,
                   jitter_mean=.2, jitter_sigma=.1,
                   sensitivity_mean=.125, sensitivity_sigma=.04,
                   threshold_mean=.4, threshold_sigma=.04):
    t = elastic_transform(255 - image, alpha=elastic_alpha, sigma=elastic_sigma)
    t = 255 - jitter(t, jitter_mean, jitter_sigma)
    t = adjust_sensitivity(t, sensitivity_mean, sensitivity_sigma)
    return threshold(t, threshold_mean, threshold_sigma)

def render_and_degrade(text, font):
    return degrade(ndimage.zoom(render(text, font), .3))

if len(sys.argv) != 5:
    print "Usage: render-and-degrade.py <text-file> <font.ttf> <quantity> <output_prefix>"
    exit(1)

text = codecs.open(sys.argv[1],'r','utf8')
text = [line.rstrip() for line in text]

font_name = sys.argv[2]
prefix = sys.argv[4] + ".files"
list = open(sys.argv[4], "w")
font = ImageFont.truetype(font_name, 100)
if not os.path.isdir(prefix):
    os.mkdir(prefix)
N = int(sys.argv[3])

for counter in range(N):
    i = randrange(len(text))
    img_line = render_and_degrade(text[i], font)
    
    min_img_line = img_line.min()
    max_img_line = img_line.max()
    img_line = 255 * (img_line - min_img_line) / (max_img_line - min_img_line)

    filename = "%s/%05d" % (prefix, counter)
    Image.fromarray(img_line.astype('uint8'), 'L').save(filename + '.png')
    transcript = codecs.open(filename + '.txt', 'w', 'utf-8')
    transcript.write(text[i])
    transcript.write('\n')
    transcript.close()
    list.write(filename + '.png ' + filename + '.txt\n')
list.close()
