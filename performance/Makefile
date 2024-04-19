TESTS=$(sort $(basename $(wildcard *.lama)))
RUNTIME=LAMA=../runtime 
RESULT=../perfomance_comparison.txt
ITERATIVE_INTERPRETER=../build/interpreter

LAMAC=lamac

.PHONY: check $(TESTS)

all: %lama_bin

performance: $(TESTS)

%.bc: %.lama 
	$(LAMAC) -b $<

%lama_bin: %.lama
	$(RUNTIME) $(LAMAC) $<	

$(TESTS): %: %.bc %lama_bin
	@echo $@
	$(RUNTIME) echo "0" | `which time` -o $(RESULT) -a -f "LAMA RECURSIVE INTERPRETER: \t\t%U" $(LAMAC) -i $@.lama 
	`which time` -o $(RESULT) -a -f "LAMA ITERATIVE INTERPRETER: \t%U" $(ITERATIVE_INTERPRETER) $@.bc 

clean:
	$(RM) test*.log *.bc *.s *~ $(TESTS) *.i $(RESULT)