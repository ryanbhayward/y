bin_PROGRAMS = y

y_SOURCES = \
board.cpp \
connect.cpp \
genmove.cpp \
interact.cpp \
move.cpp \
node.cpp \
play.cpp \
shuff.cpp \
ui.cpp \
YMain.cpp \
YGtpEngine.cpp \
YSearch.cpp \
YUctSearch.cpp

noinst_HEADERS = \
board.h \
connect.h \
genmove.h \
interact.h \
move.h \
node.h \
shuff.h \
ui.h \
YGtpEngine.h \
YSearch.h \
YUctSearch.h

y_LDADD = \
$(FUEGO_BUILD)/smartgame/libfuego_smartgame.a \
$(FUEGO_BUILD)/gtpengine/libfuego_gtpengine.a \
$(BOOST_PROGRAM_OPTIONS_LIB) \
$(BOOST_FILESYSTEM_LIB) \
$(BOOST_SYSTEM_LIB) \
$(BOOST_THREAD_LIB)

y_DEPENDENCIES = \
$(FUEGO_BUILD)/smartgame/libfuego_smartgame.a \
$(FUEGO_BUILD)/gtpengine/libfuego_gtpengine.a

y_LDFLAGS = $(BOOST_LDFLAGS)

y_CPPFLAGS = \
$(BOOST_CPPFLAGS) \
-I$(FUEGO_ROOT)/smartgame \
-I$(FUEGO_ROOT)/gtpengine \
-I@top_srcdir@/src

DISTCLEANFILES = *~
