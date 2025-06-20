PROGNAME = ret2spec_sa

all: $(PROGNAME)

clean:
	rm -f *.o ret2spec_sa

$(PROGNAME): ret2spec_sa.o cache_sidechannel.o utils.o
	clang++ -g ret2spec_sa.cc cache_sidechannel.cc asm/measurereadlatency_x86_64.S utils.cc ret2spec_common.cc  -O3 -o ret2spec_sa


.PHONY: all cleanmeasure