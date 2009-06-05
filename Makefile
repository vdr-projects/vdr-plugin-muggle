#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = muggle

all: libvdr-$(PLUGIN).so mugglei i18n


#no HAVE_* flags should ever be changed in this Makefile. Instead
#edit $VDRDIR/Make.config

#if you want ogg / flac support, define HAVE_VORBISFILE and/or HAVE_FLAC
#in $VDRDIR/Make.config like this:
#HAVE_VORBISFILE=1
#HAVE_FLAC=1
#HAVE_SNDFILE=1

HAVE_FFARD=1

#if you do not want to compile in code for embedded mysql,
#define this:
#HAVE_ONLY_SERVER=1

# Define what database you want to use, one of MySQL (HAVE_MYSQL=1),
# PostgreSQL (HAVE_PG=1) or SQLite (HAVE_SQLITE=1).  Default is MySQL.
#HAVE_SQLITE = 1
#HAVE_PG = 1
HAVE_MYSQL = 1

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char VERSION\[\] *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -fPIC -O0 -Wall -Wformat=2 -Woverloaded-virtual -Wno-deprecated -g

### The directory environment:

VDRDIR ?= ../../..
LIBDIR ?= ../../lib
TMPDIR ?= /tmp
BINDIR ?= /usr/local/bin

MUSICDIR ?= /mnt/music

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

ifdef HAVE_SQLITE
HAVE_MYSQL =
HAVE_ONLY_SERVER =
HAVE_PG =
else
ifdef HAVE_PG
HAVE_MYSQL =
HAVE_ONLY_SERVER =
HAVE_SQLITE = 
else
HAVE_MYSQL = 1
endif
endif

### The version number of VDR (taken from VDR's "config.h"):

APIVERSION = $(shell (grep 'define APIVERSION ' $(VDRDIR)/config.h || grep 'define VDRVERSION ' $(VDRDIR)/config.h) | awk '{ print $$3 }' | sed -e 's/"//g')

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

FTWNOTWANTED = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
DEFINES := $(filter-out $(FTWNOTWANTED),$(DEFINES))

INCLUDES += -I$(VDRDIR) -I$(VDRDIR)/include \
	$(shell taglib-config --cflags)

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
DEFINES += -DMUSICDIR='"$(MUSICDIR)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o mg_valmap.o mg_thread_sync.o \
	mg_item.o mg_item_gd.o mg_listitem.o mg_selection.o mg_sel_gd.o vdr_actions.o mg_menu.o vdr_menu.o mg_tools.o \
	vdr_decoder_mp3.o vdr_stream.o vdr_decoder.o vdr_player.o \
	vdr_setup.o mg_setup.o mg_incremental_search.o mg_image_provider.o \
	mg_skin.o quantize.o mg_playcommands.o pcmplayer.o \
	lyrics.o bitmap.o imagecache.o

PLAYLIBS = -lmad $(shell taglib-config --libs) -lImlib2 
MILIBS =  $(shell taglib-config --libs)


ifdef HAVE_SQLITE
DB_OBJ = mg_db_gd_sqlite.o
DB_CFLAGS = $(shell pkg-config --cflags sqlite3)
SQLLIBS = $(shell pkg-config --libs sqlite3)
DEFINES += -DHAVE_SQLITE
endif

ifdef HAVE_MYSQL
DB_OBJ = mg_db_gd_mysql.o
DB_CFLAGS = $(shell mysql_config --cflags)
DEFINES += -DHAVE_MYSQL
ifdef HAVE_ONLY_SERVER
SQLLIBS = $(shell mysql_config --libs)
DEFINES += -DHAVE_ONLY_SERVER
else
SQLLIBS = $(shell mysql_config --libmysqld-libs) 
endif
endif

ifdef HAVE_PG
DB_OBJ = mg_db_gd_pg.o
DB_CFLAGS = -I$(shell pg_config --includedir) 
SQLLIBS = -L$(shell pg_config --libdir) -lpq
DEFINES += -DHAVE_PG
endif

DB_OBJ += mg_db.o

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

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $<

$(DB_OBJ): CXXFLAGS += $(DB_CFLAGS)

### Dependencies:
mg_tables.h:	scripts/gentables scripts/genres.txt scripts/iso_639.xml scripts/musictypes.txt scripts/sources.txt
	scripts/gentables > $@

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP -ktrdb --msgid-bugs-address='<vdr-muggle-develop@sourceforge.net>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs) $(I18Npot)


### Targets:

libvdr-$(PLUGIN).so: $(OBJS) $(MOBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(MOBJS) $(PLAYLIBS) $(MLIBS) $(SQLLIBS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)
	chmod 644 $(LIBDIR)/$@.$(APIVERSION)

mugglei: mg_tools.o mugglei.o $(DB_OBJ) mg_listitem.o mg_item.o mg_item_gd.o mg_valmap.o mg_setup.o 
	$(CXX) $(CXXFLAGS) $^ $(MILIBS) $(SQLLIBS) -o $@

install: all
	@cp --remove-destination $(LIBDIR)/libvdr-muggle*.so.* \
		$(DESTDIR)$(PLUGINLIBDIR)
	@chmod 644 $(DESTDIR)$(PLUGINLIBDIR)/libvdr-muggle*.so*
	@cp mugglei $(DESTDIR)$(BINDIR)/
	@cp scripts/muggle-image-convert $(DESTDIR)$(BINDIR)/

dist: distclean mg_tables.h
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude=.svn -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(MOBJS) $(BINOBJS) $(DEPFILE) *.so *.tgz core* *~ mugglei.o mugglei mg_db_gd_*.o

distclean: clean
	@-rm -f mg_tables.h
