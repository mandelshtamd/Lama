TESTS=$(sort $(filter-out test111, $(basename $(wildcard test*.lama))))

LAMAC=lamac
ITER_INTERPRETER=../build/interpreter

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.bc
	@echo "regression/$@ "
	$(ITER_INTERPRETER) $< < $@.input > $@.log && diff $@.log orig/$@.log


%.bc: %.lama 
	$(LAMAC) -b $<

clean:
	$(RM) *.bc test*.log *.s *.sm *~ $(TESTS) *.i 
	$(MAKE) clean -C expressions
	$(MAKE) clean -C deep-expressions
