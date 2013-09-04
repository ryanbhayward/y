bin_PROGRAMS = y

y_SOURCES = \
board.cpp \
ConstBoard.cpp \
Groups.cpp \
SemiTable.cpp \
WeightedRandom.cpp \
YMain.cpp \
YGtpEngine.cpp \
YSearch.cpp \
YSgUtil.cpp \
YUctSearch.cpp \
YUctSearchUtil.cpp \
YUtil.cpp

noinst_HEADERS = \
board.h \
ConstBoard.h \
Groups.h \
SemiTable.h \
VectorIterator.h \
WeightedRandom.h \
YGtpEngine.h \
YSearch.h \
YSgUtil.h \
YUctSearch.h \
YUctSearchUtil.h \
YUtil.h

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
