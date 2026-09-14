# Run from src, with the same configuration as the existing object files:
# make -f Makefile -f ../tests/spark_state.mk ARCH=x86-64 spark-state
.PHONY: spark-state
spark-state: $(filter-out main.o,$(OBJS)) ../tests/spark_state.cpp
	$(CXX) $(CXXFLAGS) -I. ../tests/spark_state.cpp $(filter-out main.o,$(OBJS)) $(LDFLAGS) -o spark-state-test
	./spark-state-test
