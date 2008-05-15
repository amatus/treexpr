# Commands to use
# for linux or freebsd or probably cygwin:
CC = gcc
RM = rm -f
JAVA = java
JAVAC = javac
JAVAH = javah

# Shared object prefix and suffix
# for linux:
LIB = lib
DOTSO = .so
# for windows or cygwin:
# LIB = 
# DOTSO = .dll

# Source files
SOURCES = regex/regcomp.c regex/regerror.c regex/regexec.c regex/regfree.c \
	treexpr.c
JNISOURCES = $(SOURCES) GrokHtml.c

# libxml2 flags
XMLINCL = $(shell xml2-config --cflags)
XMLLIBS = $(shell xml2-config --libs)

# java flags
# for linux using SUN jdk15:
# JAVAINCL = -I/opt/jdk-1.5.0/include -I/opt/jdk-1.5.0/include/linux
# for linux using sablevm:
JAVAINCL = -I/usr/include/sablevm
# for freebsd using jdk15 from portage:
# JAVAINCL = -I/usr/local/jdk1.5.0/include -I/usr/local/jdk1.5.0/include/freebsd

INCL = -I./regex $(XMLINCL)
LIBS = $(XMLLIBS)
CFLAGS += -O2 -Wall

all: $(LIB)GrokHtml$(DOTSO) $(LIB)treexpr$(DOTSO)

GrokHtml.h: GrokHtml.class
	$(JAVAH) -classpath . -jni -o $@ GrokHtml

GrokHtml.class: GrokHtml.java
	$(JAVAC) $<

TestIt.class: TestIt.java
	$(JAVAC) $<

$(LIB)GrokHtml$(DOTSO): $(JNISOURCES) GrokHtml.h
	$(CC) $(LIBS) $(CFLAGS) $(INCL) $(JAVAINCL) -shared -o $@ $(JNISOURCES)

$(LIB)treexpr$(DOTSO): $(SOURCES)
	$(CC) $(LIBS) $(CFLAGS) $(INCL) -shared -o $@ $(SOURCES)

run: run.c $(LIB)treexpr$(DOTSO) treexpr.h
	$(CC) $(LIBS) $(CFLAGS) $(INCL) $(LIB)treexpr$(DOTSO) -o $@ run.c

test: $(LIB)GrokHtml$(DOTSO) TestIt.class
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):. $(JAVA) TestIt

clean:
	$(RM) $(LIB)GrokHtml$(DOTSO) 
	$(RM) $(LIB)treexpr$(DOTSO) 
	$(RM) GrokHtml.h
	$(RM) *.class
