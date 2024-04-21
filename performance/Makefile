TESTS=$(sort $(basename $(wildcard *.lama)))
TESTS_ANALYSIS=$(addprefix analysis, $(TESTS))
RUNTIME=LAMA=../runtime 
RESULT=../perfomance_comparison.txt
BYTECODE_ANALYSIS=../build/bytecode_analysis
ITERATIVE_INTERPRETER=../build/interpreter

LAMAC=lamac

.PHONY: check $(TESTS)

all: %lama_bin

performance: $(TESTS)

bytecode_analysis: $(TESTS_ANALYSIS)

%.bc: %.lama 
	$(LAMAC) -b $<

%lama_bin: %.lama
	$(RUNTIME) $(LAMAC) $<	

$(TESTS): %: %.bc %lama_bin
	@echo $@
	$(RUNTIME) echo "0" | `which time` -o $(RESULT) -a -f "LAMA RECURSIVE INTERPRETER: \t\t%U" $(LAMAC) -i $@.lama 
	`which time` -o $(RESULT) -a -f "LAMA ITERATIVE INTERPRETER: \t%U" $(ITERATIVE_INTERPRETER) $@.bc 

$(TESTS_ANALYSIS): analysis% : %.bc 
	@echo "test bytecode analysis " 
	$(BYTECODE_ANALYSIS) $(patsubst analysis%,%,$@).bc

clean:
	$(RM) test*.log *.bc *.s *~ $(TESTS) *.i $(RESULT)