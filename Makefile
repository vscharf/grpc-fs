PROTOSDIR = protos
GENDIR = src_gen
OBJDIR = objs
BINDIR = bins

vpath %.proto $(PROTOSDIR)
vpath %.pb.cc $(GENDIR)
vpath %.pb.h $(GENDIR)
vpath %.cc src/server src/client

CXX = g++
CPPFLAGS += -pthread -I$(GENDIR) -g
CXXFLAGS += -std=c++11
LDFLAGS += -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGINDIR ?= `which $(GRPC_CPP_PLUGIN)`

all: $(BINDIR)/grpcfs_server $(BINDIR)/grpcfs_client

test: $(addprefix tests/performance-tests-local/,transfer_ram_ram transfer_ram_ram_parallel transfer_disk_disk)

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

tests/%: grpc_fs.pb.o tests/%.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(GENDIR)/*.pb.cc $(GENDIR)/*.pb.h

.PHONY: binclean
binclean:
	rm -f $(BINDIR)/*
