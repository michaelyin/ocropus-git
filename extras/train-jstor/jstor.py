#!/usr/bin/python
# -*- coding: iso-8859-1 -*-

import string,re,sys,os,shutil,glob
import xml.sax as sax
from PIL import Image

usage = """
... hocr image_pattern output_prefix



Arguments:

    xml_pattern: either a glob pattern that results in a list
        of xml files, or @filename for a file containing
        a list of image files in order; DON'T FORGET TO QUOTE THIS

    image_pattern: either a glob pattern that results in a list
        of image files, or @filename for a file containing
        a list of image files in order; DON'T FORGET TO QUOTE THIS

    output_pattern: output images are of the form
        output_pattern%(pageno,lineno)

Environment Variables:

    element="ln": which element to extract; ln or wd

    regex=".": the text for any transcription must match this pattern

    dict=None: a dictionary; if provided, all the words in any line
        that's output by the program must occur in the dictionary

    min_len=5: minimum length of text for which lines are output

    max_len=200: maximum length of text for which lines are output

    max_lines=1000000: maximum number of lines output

    pad=2: pad the bounding box by this many pixels prior to extraction

    output_format=png: format for line image files
"""

if len(sys.argv)!=4:
    sys.stderr.write(usage)
    print "args:",sys.argv
    sys.exit(1)

exe,xml_pattern,image_pattern,output_pattern = sys.argv

if image_pattern[0]=="@":
    image_list = open(image_pattern[1:]).readlines()
    image_list = [s[:-1] for s in image_list]
    image_list.sort()
else:
    image_list = glob.glob(image_pattern)
    image_list.sort()

if xml_pattern[0]=="@":
    xml_list = open(xml_pattern[1:]).readlines()
    xml_list = [s[:-1] for s in xml_list]
    xml_list.sort()
else:
    xml_list = glob.glob(xml_pattern)
    xml_list.sort()

if len(image_list) != len(xml_list):
    print "different number of images and xml files"
    sys.exit(-1);

element = os.getenv("element","ln")
regex = os.getenv("regex",".")
min_len = int(os.getenv("min_len","5"))
max_len = int(os.getenv("max_len","200"))
dict = None
dictfile = os.getenv("dict")
max_lines = int(os.getenv("max_lines","1000000"))
pad = int(os.getenv("pad","2"))
output_format = os.getenv("output_format","png")

if dictfile:
    stream = open(dictfile,"r")
    words = stream.read().split()
    stream.close()
    dict = {}
    for word in words: dict[word.lower()] = 1
    # print "[read %d words from %s]\n"%(len(words),dictfile)

def check_dict(dict,s):
    if not dict: return 1
    words = re.split(r'\W+',s)
    for word in words:
        if word=="": continue
        if not dict.get(word.lower()): return 0
    return 1

def write_string(file,text):
    stream = open(file,"w")
    stream.write(text.encode("utf-8"))
    stream.close()

def get_prop(title,name):
    props = title.split(';')
    for prop in props:
        (key,args) = prop.split(None,1)
        if key==name: return args
    return None

class DocHandler(sax.handler.ContentHandler):
    def __init__(self, pageno, image):
        self.element = element
        self.regex = regex
        self.pageno = pageno
        self.image = image
        self.p = re.compile("^( )*$")

    def startDocument(self):
        self.lineno = 0
        self.total = 0
        self.text = None

    def endDocument(self):
        pass

    def startElement(self,name,attrs):
        if name == self.element:
            if name == "ln":
                self.l = int(attrs.get("lx",""))
                self.t = int(attrs.get("ly",""))
                self.r = int(attrs.get("rx",""))
                self.b = int(attrs.get("ry",""))
            if name == "wd":
                self.l = int(attrs.get("l",""))
                self.t = int(attrs.get("t",""))
                self.r = int(attrs.get("r",""))
                self.b = int(attrs.get("b",""))
            self.text = u""
            self.lineno += 1

    def endElement(self,name):
        if name == self.element:
            if len(self.text) >= min_len and \
                    len(self.text) <= max_len and \
                    re.match(self.regex,self.text) and \
                    check_dict(dict,self.text):
                w,h = self.image.size
                x0 = max(0,self.l-pad)
                y0 = max(0,self.t-pad)
                x1 = min(w,self.r+pad)
                y1 = min(h,self.b+pad)
                limage = self.image.crop((x0,y0,x1,y1))
                base = output_pattern%(self.pageno, self.lineno)
                basedir = os.path.dirname(base)
                if not os.path.exists(basedir):
                    os.makedirs(basedir)
                imgFile = base + "." + output_format
                txtFile = base+".gt.txt"
                limage.save(imgFile)
                write_string(txtFile, self.text)
                #write_string(base+".bbox",self.bbox)
                print x0, y0, x1, y1
                print imgFile
                print txtFile
                print self.text.encode("utf-8")
                print 
                self.total += 1
                if self.total >= max_lines:
                    sys.exit(0)
            self.text = None

    def characters(self, content):
        if self.text != None and not self.p.match(content):
            if self.text != u"":
                self.text += " "
            self.text += content

print "element:        " , element
print "regex:          " , regex
print "min_len:        " , min_len
print "max_len:        " , max_len
print "max_lines:      " , max_lines
print "pad:            " , pad
print "dict:           " , dict
print "output_format:  " , output_format

for i in range(len(image_list)):
    #stream = os.popen("tidy -q -wrap 9999 -asxhtml < %s 2> /tmp/tidy_errs" % xml_list[i],"r")
    pageno = i+1
    print "\n\n%04d" % pageno, xml_list[i], image_list[i]
    
    parser = sax.make_parser()
    parser.setContentHandler(DocHandler(pageno, Image.open(image_list[i])))
    stream = open(xml_list[i],"r")
    parser.parse(stream)
    stream.close()
