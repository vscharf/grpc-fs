PROTOSDIR = protos
GENDIR = src_gen
OBJDIR = objs
BINDIR = bins

vpath %.proto $(PROTOSDIR)
vpath %.pb.cc $(GENDIR)
vpath %.pb.h $(GENDIR)
vpath %.cc src/server src/client

CXX = g++
CPPFLAGS += -pthread -I$(GENDIR)
CXXFLAGS += -std=c++11
LDFLAGS += -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGINDIR ?= `which $(GRPC_CPP_PLUGIN)`

all: system-check $(BINDIR)/grpcfs_server $(BINDIR)/grpcfs_client

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

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(GENDIR)/*.pb.cc $(GENDIR)/*.pb.h

.PHONY: binclean
binclean:
	rm -f $(BINDIR)/*

# The following is to test your system and ensure a smoother experience.
# They are by no means necessary to actually compile a grpc-enabled software.

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0-alpha-1"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif
