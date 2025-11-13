CXX?=		g++

CXXFLAGS?=	 -std=c++20 \
		-Wall \
		-Iinclude \
		-Ithird_party/pfi/include \
		-g
LDFLAGS?=
LDADD=-lz

TARGET=		kill_proxy

CXX_SRCS=	src/kill_proxy.cpp
CXX_OBJS=	$(CXX_SRCS:%.cpp=%.cpp.o)
CXX_DEPS=	$(CXX_SRCS:%.cpp=%.cpp.d)

all: ${TARGET}

${TARGET}: ${CXX_OBJS}
	${CXX} ${LDFLAGS} -o $@ $< ${LDADD}

%.cpp.o: %.cpp
	${CXX} ${CXXFLAGS} -MMD -c -o $@ $<


-include $(CXX_DEPS)

.PHONY: clean
clean:
	-rm ${TARGET} ${CXX_OBJS} ${CXX_DEPS}
