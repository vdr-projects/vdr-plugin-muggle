#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = muggle

#no HAVE_* flags should ever be changed in this Makefile. Instead
#edit $VDRDIR/Make.config

#if you want ogg / flac support, define HAVE_VORBISFILE and/or HAVE_FLAC
#in $VDRDIR/Make.config like this:
#HAVE_VORBISFILE=1
#HAVE_FLAC=1
#HAVE_SNDFILE=1

#if you do not want to compile in code for embedded mysql,
#define this:
# HAVE_ONLY_SERVER=1

#define what database you want to use. Default is mysql. HAVE_SQLITE
#removes mysql support and adds SQLite support
# HAVE_SQLITE = 1

HAVE_MYSQL = 1

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++-3.3
CXXFLAGS ?= -fPIC -O0 -Wall -Woverloaded-virtual -Wno-deprecated -g 

### The directory environment:

DVBDIR ?= ../../../../DVB
VDRDIR ?= ../../../
LIBDIR ?= ../../lib
TMPDIR ?= /tmp

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

ifdef HAVE_SQLITE
HAVE_MYSQL =
HAVE_ONLY_SERVER =
else
ifdef HAVE_PG
HAVE_MYSQL =
HAVE_SQLITE = 
HAVE_ONLY_SERVER =
endif
endif

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR) -I$(VDRDIR)/include -I$(DVBDIR)/include \
	$(shell taglib-config --cflags)

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o i18n.o mg_valmap.o mg_db.o mg_thread_sync.o \
	mg_item.o mg_item_gd.o mg_listitem.o mg_selection.o mg_sel_gd.o vdr_actions.o vdr_menu.o mg_tools.o \
	vdr_decoder_mp3.o vdr_stream.o vdr_decoder.o vdr_player.o \
	vdr_setup.o mg_setup.o mg_incremental_search.o mg_image_provider.o

PLAYLIBS = -lmad $(shell taglib-config --libs)
MILIBS =  $(shell taglib-config --libs)

ifdef HAVE_SQLITE
SQLLIBS += -lsqlite3
DB_OBJ = mg_db_gd_sqlite.o
DEFINES += -DHAVE_SQLITE
endif

ifdef HAVE_MYSQL
INCLUDES += $(shell mysql_config --cflags) 
DB_OBJ = mg_db_gd_mysql.o
DEFINES += -DHAVE_MYSQL
ifdef HAVE_ONLY_SERVER
SQLLIBS =  $(shell mysql_config --libs)
DEFINES += -DHAVE_ONLY_SERVER
else
SQLLIBS = $(shell mysql_config --libmysqld-libs) 
endif
endif

ifdef HAVE_PG
INCLUDES += -I$(shell pg_config --includedir) 
SQLLIBS =  -L$(shell pg_config --libdir) -lpq
DB_OBJ = mg_db_gd_pg.o
DEFINES += -DHAVE_PG
endif

ifdef HAVE_VORBISFILE
DEFINES += -DHAVE_VORBISFILE
OBJS += vdr_decoder_ogg.o
PLAYLIBS += -lvorbisfile -lvorbis
endif

ifdef HAVE_FLAC
DEFINES += -DHAVE_FLAC
OBJS += vdr_decoder_flac.o
PLAYLIBS += -lFLAC++ -lFLAC
endif

ifdef HAVE_SNDFILE
DEFINES += -DHAVE_SNDFILE
OBJS += vdr_decoder_sndfile.o
PLAYLIBS += -lsndfile
endif


OBJS += $(DB_OBJ)

### Targets:

all: libvdr-$(PLUGIN).so mugglei

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $<

mg_tables.h:	scripts/genres.txt scripts/iso_639.xml scripts/musictypes.txt scripts/sources.txt
	scripts/gentables

libvdr-$(PLUGIN).so: $(OBJS) $(DB_OBJ)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(PLAYLIBS) $(SQLLIBS) -o $@
	@cp $@ $(LIBDIR)/$@.$(VDRVERSION)

mugglei: mg_tools.o mugglei.o mg_db.o $(DB_OBJ) mg_listitem.o mg_item.o mg_item_gd.o mg_valmap.o mg_setup.o 
	$(CXX) $(CXXFLAGS) $^ $(MILIBS) $(SQLLIBS) -o $@

install:
	@cp ../../lib/libvdr-muggle*.so.* /usr/lib/vdr/
	@cp mugglei /usr/local/bin/
#	@install -m 755 mugglei /usr/local/bin/

dist: clean mg_tables.h
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude=.svn/* -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(BINOBJS) $(DEPFILE) *.so *.tgz core* *~ mugglei.o mugglei
