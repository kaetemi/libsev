#!/usr/bin/python
# 
# Copyright (C) 2008-2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 

import time, sys
from textwrap import TextWrapper

wrapper = TextWrapper()
indent = " * "
wrapper.initial_indent = indent
wrapper.subsequent_indent = indent
gmtime = time.gmtime(time.time())
filename = "NEEDED_FOR_buildLine"
newline = "\n"

print ""

print "This script will create .cpp and .h files for your class."
print "To use the defaults, simply hit ENTER, else type in the new value."

print ""

def askVar(name, default):
	sys.stdout.write(name + " (" + default + "): ")
	line = sys.stdin.readline()
	linestrip = line.strip()
	if linestrip == "":
		return default
	else:
		return linestrip

file = askVar("File", "your_class_name").replace(" ", "_")
classname = askVar("Class", "YourClassName")
author = askVar("Author NAME", "Jan BOON")
username = askVar("Username", "Kaetemi")
email = askVar("E-mail", "jan.boon@kaetemi.be")
namespace = askVar("Namespace", "sev")
product = askVar("Product", "Simple Event Library (LibSEv)")
dir = askVar("Directory", "./")
year = askVar("Year", time.strftime("%Y", gmtime))
date = askVar("Date", time.strftime("%Y-%m-%d %H:%MGMT", gmtime))
hdefine = askVar("Define", namespace.upper() + "_" + file.upper() + "_H")

print ""

license = [ ]
with open('license.txt') as f:
	license = [i.rstrip('\n') for i in f.readlines()]

header = [ ]
with open('class.h') as f:
	header = [i.rstrip('\n') for i in f.readlines()]

source = [ ]
with open('class.cpp') as f:
	source = [i.rstrip('\n') for i in f.readlines()]

def buildLine(line):
	newline = line.replace("YEAR", year)
	newline = newline.replace("DATE", date)
	newline = newline.replace("Author NAME", author)
	newline = newline.replace("Username", username)
	newline = newline.replace("mail@example.com", email)
	newline = newline.replace("ClassName", classname)
	newline = newline.replace("className", classname[0].lower() + classname[1:])
	newline = newline.replace("class_name", file)
	newline = newline.replace("NMSP_CLASSNAME_H", hdefine)
	newline = newline.replace("NMSP", namespace.upper())
	newline = newline.replace("nmsp", namespace.lower())
	return newline

def printComment(f, lines):
	for line in lines:
		if line == "":
			f.write(indent + newline)
		else:
			for subline in wrapper.wrap(buildLine(line)):
				f.write(subline + newline)

def writeHeader(f):
	f.write("/**" + newline)
	printComment(f, filedoc)
	f.write(" */" + newline)
	f.write(newline)
	f.write("/* " + newline)
	printComment(f, copyright)
	f.write(" */" + newline)
	f.write(newline)

def writeCode(f, code):
	for line in code:
		f.write(buildLine(line) + newline)

#note: need filename for buildLine
filename = file + ".cpp"
filepath = dir + filename
f = open(filepath, 'w')
f.write("/*" + newline + newline)
writeCode(f, license)
f.write(newline + "*/" + newline)
writeCode(f, source)
f.close()

filename = file + ".h"
filepath = dir + filename
f = open(filepath, 'w')
f.write("/*" + newline + newline)
writeCode(f, license)
f.write(newline + "*/" + newline)
writeCode(f, header)
f.close()

print "Done."
