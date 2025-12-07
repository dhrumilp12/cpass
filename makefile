INPUT       = input1
VERBOSE     = 0
OPT_SO      = build/store_prop/libstore_prop.so
REF_OPT_SO  = ref_lib/libstore_prop.so

INPUTS_DIR  = inputs
IR_DIR      = ir
EXE_DIR     = exe

UNOPT_LL    = $(IR_DIR)/unopt/$(INPUT).ll
OPT_LL      = $(IR_DIR)/opt/$(INPUT).ll
REF_OPT_LL  = $(IR_DIR)/ref_opt/$(INPUT).ll

UNOPT_EXE   = $(EXE_DIR)/unopt/$(INPUT)_exe
OPT_EXE     = $(EXE_DIR)/opt/$(INPUT)_exe
REF_OPT_EXE = $(EXE_DIR)/ref_opt/$(INPUT)_exe

VERBOSE_FLAGS =
ifeq ($(VERBOSE),1)
VERBOSE_FLAGS = -store-prop-verbose
endif

all: $(OPT_SO)

$(OPT_SO):
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C build

unopt_ll: $(UNOPT_LL)
ref_opt_ll: $(REF_OPT_LL)
opt_ll: $(OPT_LL)

$(UNOPT_LL):
	clang -S -emit-llvm -O0 $(INPUTS_DIR)/$(INPUT).c -o $@

$(REF_OPT_LL): $(UNOPT_LL)
	opt -load-pass-plugin $(REF_OPT_SO) \
	    -passes='default<O0>,module(remove-optnone),function(store-prop)' \
		$(VERBOSE_FLAGS) < $(UNOPT_LL) | llvm-dis -o $@

$(OPT_LL): $(OPT_SO) $(UNOPT_LL)
	opt -load-pass-plugin $(OPT_SO) \
	    -passes='default<O0>,module(remove-optnone),function(store-prop)' \
		$(VERBOSE_FLAGS) < $(UNOPT_LL) | llvm-dis -o $@

unopt_exe: $(UNOPT_EXE)
ref_opt_exe: $(REF_OPT_EXE)
opt_exe: $(OPT_EXE)

$(UNOPT_EXE): $(UNOPT_LL)
$(REF_OPT_EXE): $(REF_OPT_LL)
$(OPT_EXE): $(OPT_LL)

$(UNOPT_EXE) $(REF_OPT_EXE) $(OPT_EXE):
	llc -filetype=obj $(subst _exe,.ll,$(patsubst exe/%,ir/%,$@)) -o $(subst _exe,.o,$@)
	clang $(subst _exe,.o,$@) -o $@
	rm -f $(subst _exe,.o,$@)

clean_exe:
	rm -f $(EXE_DIR)/unopt/*
	rm -f $(EXE_DIR)/opt/*
	rm -f $(EXE_DIR)/ref_opt/*

clean_ir: clean_exe
	rm -f $(IR_DIR)/unopt/*.ll
	rm -f $(IR_DIR)/opt/*.ll
	rm -f $(IR_DIR)/ref_opt/*.ll

clean: clean_ir
	rm -rf build/*
