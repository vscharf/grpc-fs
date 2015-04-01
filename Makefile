PROTOSDIR = protos
GENDIR = src_gen
OBJDIR = objs
BINDIR = bins

vpath %.proto $(PROTOSDIR)
vpath %.pb.cc $(GENDIR)
vpath %.pb.h $(GENDIR)
vpath %.cc src/server src/client

CXX = g++
CPPFLAGS += -pthread -g -I$(GENDIR) -I$(BOOST_INCLUDEDIR) -DBOOST_DATE_TIME_NO_LIB -I../grpc
CXXFLAGS += -std=c++11
LDFLAGS_STATIC += -static -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -Wl,-Bdynamic -lpthread -ldl -lrt
LDFLAGS += -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGINDIR ?= `which $(GRPC_CPP_PLUGIN)`

TESTS = tests/performance-tests-local/transfer_ram_ram \
	tests/performance-tests-local/transfer_ram_ram_parallel \
	tests/performance-tests-local/transfer_disk_disk \
	tests/performance-tests-remote/transfer_disk_disk

BINS = $(BINDIR)/grpcfs_server \
       $(BINDIR)/grpcfs_client

all: $(BINS) $(addsuffix _static, $(BINS))
#all: $(BINDIR)/grpcfs_client

test: $(TESTS) $(addsuffix _static, $(TESTS))

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
