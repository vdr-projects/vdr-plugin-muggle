#
# Makefile for a Video Disk Recorder plugin
#
# $Id: Makefile,v 1.9 2004/05/28 15:29:18 lvw Exp $

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = muggle

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++-3.3
CXXFLAGS ?= -O2 -Wall -Woverloaded-virtual -Wno-deprecated -g

### The directory environment:

DVBDIR = ../../../../DVB
VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR) -I$(VDRDIR)/include -I$(DVBDIR)/include -I/usr/include/mysql/

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o i18n.o vdr_menu.o mg_database.o mg_content_interface.o gd_content_interface.o mg_tools.o mg_media.o mg_filters.o mg_playlist.o vdr_decoder_mp3.o vdr_stream.o vdr_decoder.o vdr_player.o vdr_setup.o

LIBS = -lmad -lmysqlclient

### Targets:

all: libvdr-$(PLUGIN).so

# Dependencies:

MAKEDEP = g++ -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Implicit rules:

%.o: %.c %.h
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	@cp $@ $(LIBDIR)/$@.$(VDRVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(BINOBJS) $(DEPFILE) *.so *.tgz core* *~
	@-rm -f sh_muggle sh_muggle2

