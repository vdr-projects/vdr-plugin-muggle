#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = muggle

#if you want ogg / flac support, remove the trailing x in those two
#definitions:
define have_vorbisfile
1
endef
define have_flac
1
endef

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++-3.3
CXXFLAGS ?= -fPIC -O0 -Wall -Woverloaded-virtual -Wno-deprecated -g 

### The directory environment:

DVBDIR = ../../../../DVB
VDRDIR = ../../../
# /usr/local/src/VDR
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

INCLUDES += -I$(VDRDIR) -I$(VDRDIR)/include -I$(DVBDIR)/include \
	-I/usr/include/mysql/ -I/usr/include/taglib

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

MIFLAGS += -I/usr/include/taglib -lmysqlclient

### The object files (add further files here):

OBJS = $(PLUGIN).o i18n.o mg_valmap.o mg_order.o mg_db.o mg_actions.o vdr_menu.o mg_tools.o \
	vdr_decoder_mp3.o vdr_stream.o vdr_decoder.o vdr_player.o \
	vdr_setup.o

LIBS = -lmad -lmysqlclient 
MILIBS = -lmysqlclient -ltag

ifdef have_vorbisfile
DEFINES += -DHAVE_VORBISFILE
OBJS += vdr_decoder_ogg.o
LIBS += -lvorbisfile -lvorbis
endif
ifdef have_flac
DEFINES += -DHAVE_FLAC
OBJS += vdr_decoder_flac.o
LIBS += -lFLAC++
endif

### Targets:

all: libvdr-$(PLUGIN).so mugglei

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Implicit rules:

%.o: %.c %.h
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $<

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	@cp $@ $(LIBDIR)/$@.$(VDRVERSION)

mugglei: mg_tools.o mugglei.o
	$(CXX) $(CXXFLAGS) $^ $(MILIBS) -o $@

install:
	@cp ../../lib/libvdr-muggle*.so.* /usr/lib/vdr/
	@cp mugglei /usr/local/bin/
#	@install -m 755 mugglei /usr/local/bin/

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(BINOBJS) $(DEPFILE) *.so *.tgz core* *~ mugglei.o mugglei

