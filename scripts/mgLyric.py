#!/usr/bin/env python
import os, sys, locale

title = sys.argv[1]
artist = sys.argv[2]
outdir = sys.argv[3]

# add other possible paths here:
googlFound = False
for scriptdir in ('/usr/share/apps/amarok/scripts/Googlyrics2', \
                'NULL'):
        if os.path.isdir(scriptdir+'/sites/'):
                sys.path.append(scriptdir + "/lib/")
                sys.path.append(scriptdir + "/sites/")
                os.chdir(scriptdir)
                googlFound = True
                break 

if not googlFound:
	outfile = open(outdir + '/1.raw',"w")
	outfile.write("Googlyrics2 is not installed\nSee http://quicode.com/googlyircs2")
	outfile.close
	sys.exit(0)

Debugging = False

if Debugging:
	outlyric=["Version 1","Version 2","Version 3"]
	for idx,item in enumerate(outlyric):
		outfile = open(outdir + '/' + str(idx) + '.raw',"w")
		outfile.write(item)
		outfile.close
	sys.exit(0)

from Googlyrics import *
g = Googlyrics()

outlyric = g.find_lyrics(title, artist)
if len(outlyric) > 0:
	for idx,item in enumerate(outlyric):
		l = item.getLyric()
		if l is not None:
			if l.lyrics is not None:
				if len(l.lyrics)>2:
# if we pipe or write output to a file, python by default recodes into ascii,
# and sys.stdout.encoding is also set to ascii. But if the system
# default locale is for example utf-8, we also want the file to be
# encoded like that
					outfile = open(outdir + '/' + str(idx) + '.raw',"w")
					outfile.write(l.lyrics.encode(locale.getdefaultlocale()[1]))
					outfile.close

