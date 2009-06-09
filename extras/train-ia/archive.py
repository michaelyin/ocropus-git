#!/usr/bin/python
# -*- coding: iso-8859-1 -*-

import urllib2, zlib, gzip, StringIO, Image, threading
import sys, zipfile, os, os.path, re, tempfile, datetime, shutil
import xml.dom.minidom as dom

nThreads = 8

def getImages(siteRoot, siteName, dir=None):
    global logFile
    if dir == None:
        dir = tempfile.mkdtemp()
    try:
        link = siteRoot + "/" + siteName + "_tif.zip"
        print("try " + link)
        f = urllib2.urlopen(link)
        logFile.write(link+"\n")
        imagesType = "_tif"
    except urllib2.URLError, e:
        print("error " + str(e.code))
        if e.code == 404:
            try:
                link = siteRoot + "/" + siteName + "_jp2.zip"
                print("try " + link)
                f = urllib2.urlopen(link)
                logFile.write(link+"\n")
                imagesType = "_jp2"
            except urllib2.URLError, e1:
                print("error " + str(e1.code))
                sys.exit()
    imagesZipFilename = dir+"/"+siteName+imagesType+".zip"
    imagesZipFile = open(imagesZipFilename, 'w')
    imagesZipFile.write(f.read())
    imagesZipFile.close()
    return imagesZipFilename, imagesType

def getAbbyy(link, zipFilename=None):
    global logFile
    try:
        zipLink = link+".gz"
        print("try " + zipLink)
        f = urllib2.urlopen(zipLink)
        logFile.write(zipLink+"\n")
        abbyyZip = f.read()
        return gzip.GzipFile(fileobj=StringIO.StringIO(abbyyZip)).read()
    except urllib2.URLError, e:
        print("error " + str(e.code))
        if e.code == 404:
            try:
                zipLink = link+".zip"
                print("try " + zipLink)
                f = urllib2.urlopen(zipLink)
                logFile.write(zipLink + "\n")
                if zipFilename == None:
                    zipFile, zipFilename = tempfile.mkstemp(dir="/scratch")
                else:
                    zipFile = os.open(zipFilename, "w")
                zipFile.write(f.read())
                zipFile.close()
                zfobj = zipfile.ZipFile(zipFilename)
                xml = zfobj.read(zfobj.namelist()[0])
                os.remove(zipFilename)
                return xml
            except urllib2.URLError, e:
                print("error " + str(e.code))
                sys.exit()

def convertOne(inFile, directory, cmd, f, t, dst=None):
    inFile = os.path.join(directory, inFile)
    if dst == None:
        outFile = inFile[:-len(f)] + t;
    else:
        outFile = dst + "/" + inFile.split("/")[-1][:-len(f)] + t
    logFile.write(inFile + " => " + outFile + "\n")
    os.system(cmd % (inFile, outFile) + " 2> /dev/null > /dev/null");

class Convert(threading.Thread):
   def __init__ (self, directory, inFiles, cmd, f, t, descr, dst=None):
      threading.Thread.__init__(self)
      self.directory = directory
      self.inFiles = inFiles
      self.cmd = cmd
      self.f = f
      self.t = t
      self.descr = descr
      self.dst = dst
   def run(self):
        if(len(self.inFiles) > 0):
            sys.stdout.write("\r"+self.descr+"...")
            n = len(self.inFiles)*1.0
            i = 0.0
            for inFile in self.inFiles:
                convertOne(inFile, self.directory, self.cmd, self.f, self.t, self.dst)
                i += 100.0
                sys.stdout.write("\r%s...%.2f%%" %(self.descr, i/n))
                sys.stdout.flush()
            sys.stdout.write("\r%s...%.2f%%\n" %(self.descr, i/n))

def convertThreaded(directory, cmd, f, t, descr, n, dst=None):
    l = []
    threads = []
    for i in range(n):
        l.append([])
    i = 0
    for inFile in os.listdir(directory):
        if inFile[-len(f):] == f and inFile[-len(t):] != t:
            l[i%n].append(inFile)
            i += 1
    for i in range(n):
        thread = Convert(directory, l[i], cmd, f, t, descr, dst)
        threads.append(thread)
        thread.start()
    for thread in threads:
        thread.join()

def unzip(file, dir=None):
    print("unzip " + file)
    if dir == None:
        dir = tempfile.mkdtemp(dir="/scratch")
#    os.mkdir(dir, 0777)
    zfobj = zipfile.ZipFile(file)
    for name in zfobj.namelist():
        if name.endswith('/'):
            os.mkdir(os.path.join(dir, name))
        else:
            if name.find("/") >= 0:
                try:
                    os.mkdir(os.path.join(dir, name.split('/')[0]))
                except:
                    pass
            outfile = open(os.path.join(dir, name), 'wb')
            outfile.write(zfobj.read(name))
            outfile.close()
    return dir

def rename(srcDir, dstDir):
    for inFile in os.listdir(srcDir):
        outFile = inFile[inFile.find('_')+1:]
        inFile = os.path.join(srcDir, inFile)
        outFile = os.path.join(dstDir, outFile)
        if inFile != outFile:
            shutil.move(inFile, outFile)

siteRoot = sys.argv[1]
siteRoot = siteRoot.strip("/")
siteName = siteRoot.split("/")[-1]
outDir = sys.argv[2] + "/" + siteName
try:
    os.mkdir(outDir)
except:
    pass

logFile = open(outDir + "/log.txt", "w")
logFile.write(datetime.datetime.now().isoformat() + "\n")
logFile.write(siteRoot + "\n")

abbyyXml = getAbbyy(siteRoot + "/" + siteName + "_abbyy", outDir)

sys.stdout.write("\rcut xml into pages...")
sys.stdout.flush()
abbyyXmls = abbyyXml.split("<page")[1:]
for i in range(len(abbyyXmls)):
    abbyyXmls[i] = "<page" + abbyyXmls[i]
abbyyXmls[-1] = abbyyXmls[-1].split("</page>")[0] + "</page>"
abbyyXml = 0

numPages = len(abbyyXmls);
try:
    for pageNo in range(numPages):
        bboxes = ""
        page = dom.parseString(abbyyXmls[pageNo])
        for line in page.getElementsByTagName("line"):
            for char in line.getElementsByTagName("charParams"):
                x0 = int(char.getAttribute("l"))
                x1 = int(char.getAttribute("r"))
                y0 = int(char.getAttribute("t"))
                y1 = int(char.getAttribute("b"))
                bboxes += "%d,%d,%d,%d,%c\n" % (x0, y0, x1, y1, char.childNodes[0].data)
        path = outDir+"/%04d.bbox.txt" % (pageNo+1)
        logFile.write(path + "\n")
        bboxFile = open(path, "w")
        bboxFile.write(bboxes.encode("utf-8"))
        bboxFile.close()
        pageNo += 1
        sys.stdout.write("\rcut xml into pages...%.2f%%" % (pageNo*100.0/(numPages*1.0)))
        sys.stdout.flush()
    sys.stdout.write("\rcut xml into pages...%.2f%%\n" % (pageNo*100.0/(numPages*1.0)))

except Exception , ex:
    os.system("rm -rf " + tmpDstDir)
    logFile.close()
    raise

imagesZipFile, imagesType = getImages(siteRoot, siteName, outDir)
imagesTmpDir = unzip(imagesZipFile, outDir) + "/" + siteName + imagesType

#os.remove(imagesZipFile)

rename(imagesTmpDir, outDir);

convertThreaded(outDir, "j2k_to_image -i %s -o %s", ".jp2", ".tif", "convert jp2 to tif", nThreads)
# convertThreaded(imagesDir, "convert %s %s", ".tif", ".png", "convert tif to png", nThreads, outDir)
convertThreaded(outDir, "./pageseg %s %s", ".tif", ".bbox.txt", "pageseg", nThreads)

os.system("/usr/local/bin/ocropus trainseg " + siteName + ".model " + outDir);
