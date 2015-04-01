PROTOSDIR = protos
GENDIR = src_gen
OBJDIR = objs
BINDIR = bins
SRCDIR = src

vpath %.proto $(PROTOSDIR)
vpath %.pb.cc $(GENDIR)
vpath %.pb.h $(GENDIR)
vpath %.cc src/server src/client

CXX = g++
CPPFLAGS += -pthread -g -I$(GENDIR) -I$(SRCDIR) -I$(BOOST_INCLUDEDIR) -DBOOST_DATE_TIME_NO_LIB -I../grpc
CXXFLAGS += -std=c++11
LDFLAGS_STATIC += -static -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -Wl,-Bdynamic -lpthread -ldl -lrt -L$(BOOST_LIBRARYDIR) -lboost_system-gcc47-mt-1_53
LDFLAGS += -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl -L$(BOOST_LIBRARYDIR) -lboost_system-gcc47-mt-1_53
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGINDIR ?= `which $(GRPC_CPP_PLUGIN)`

LOCAL_TESTS    = tests/performance-tests-local/transfer_ram_ram
REMOTE_TESTS   = tests/performance-tests-local/transfer_ram_ram_parallel \
                 tests/performance-tests-local/transfer_disk_disk \
                 tests/performance-tests-remote/transfer_disk_disk
TRANSFER_TESTS = tests/transfer/transfer_local_client \
                 tests/transfer/transfer_local_server

TESTS = $(LOCAL_TESTS) $(REMOTE_TESTS) $(TRANSFER_TESTS)

BINS = $(BINDIR)/grpcfs_server \
       $(BINDIR)/grpcfs_client

all: $(BINS) $(addsuffix _static, $(BINS))
#all: $(BINDIR)/grpcfs_client

test: $(TRANSFER_TESTS) # $(addsuffix _static, $(TESTS))

$(GENDIR):
	@mkdir -p $(GENDIR)
$(OBJDIR):
	@mkdir -p $(OBJDIR)
$(BINDIR):
	@mkdir -p $(BINDIR)

%.pb.cc: %.proto $(GENDIR)
	$(PROTOC) -I $(PROTOSDIR) --cpp_out=src_gen --grpc_out=src_gen --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGINDIR) $<

$(OBJDIR)/%.pb.o: %.pb.cc | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

$(BINDIR)/%: $(OBJDIR)/grpc_fs.pb.o $(OBJDIR)/%.o | $(BINDIR)
	$(CXX) $^ $(LDFLAGS) -o $@

$(BINDIR)/%_static: $(OBJDIR)/grpc_fs.pb.o $(OBJDIR)/%.o | $(BINDIR)
	$(CXX) $^ $(LDFLAGS_STATIC) -o $@

tests/%: grpc_fs.pb.o tests/%.o
	$(CXX) $^ $(LDFLAGS) -o $@

tests/%_static: grpc_fs.pb.o tests/%.o
	$(CXX) $^ $(LDFLAGS_STATIC) -o $@

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(GENDIR)/*.pb.cc $(GENDIR)/*.pb.h

.PHONY: binclean
binclean:
	rm -f $(BINDIR)/*
