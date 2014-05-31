bin_PROGRAMS = mm_player_testsuite

mm_player_testsuite_SOURCES = mm_player_testsuite.c

mm_player_testsuite_CFLAGS = -I$(srcdir)/../src/include \
			     $(GLIB_CFLAGS)\
			     $(INIPARSER_CFLAGS) \
			     $(MMCOMMON_CFLAGS) \
			     $(MMSOUND_CFLAGS) \
			     $(MMSESSION_CFLAGS) \
			     $(ELEMENTARY_CFLAGS) \
			     $(APPFWK_CFLAGS) \
			     $(GTK_CFLAGS) \
			     $(MMTA_CFLAGS) \
			     $(MMUTIL_CFLAGS) 

############################################
#mm_player_testsuite_CFLAGS += -DAPPSRC_TEST
mm_player_testsuite_CFLAGS += -D_USE_EFL
mm_player_testsuite_CFLAGS += -DUSE_AUDIO_EFFECT

############################################

mm_player_testsuite_DEPENDENCIES = $(top_builddir)/src/libmmfplayer.la
						
mm_player_testsuite_LDADD = $(top_builddir)/src/libmmfplayer.la \
			    $(GLIB_LIBS)\
			    $(INIPARSER_LIBS)\
			    $(MMCOMMON_LIBS) \
			    $(MMSOUND_LIBS) \
			    $(MMSESSION_LIBS) \
			    $(ELEMENTARY_LIBS) \
			    $(APPFWK_LIBS) \
			    $(GTK_LIBS) \
			    $(MMTA_LIBS) \
			    $(MMUTIL_LIBS) \
			    $(ECORE_X_LIBS) \
			    $(EVAS_LIBS) \
			    $(ECORE_LIBS) \
			    $(INIPARSER_LIBS)

