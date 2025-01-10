CPP_STANDARD := c++17
CXXFLAGS := -O3 -fPIC -std=$(CPP_STANDARD)
PREFIX:=.

.DEFAULT_GOAL := all
.PHONY: clean all install

HLS4ML_INCLUDE := include/hls4ml/
INCLUDES := -I$(HLS4ML_INCLUDE)

EMULATOR_LIB:=libemulator_interface.so
all: $(EMULATOR_LIB)
	@echo All done

install: all
	@rm -rf $(PREFIX)/lib64
	@mkdir -p $(PREFIX)/lib64
	cp $(EMULATOR_LIB) $(PREFIX)/lib64
	@if [ "$(PREFIX)" != "." ] ; then rm -rf $(PREFIX)/include; cp -r include $(PREFIX)/include; fi

$(EMULATOR_LIB): src/hls4ml/emulator.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared $^ -o $@

clean:
	rm -f $(EMULATOR_LIB)
