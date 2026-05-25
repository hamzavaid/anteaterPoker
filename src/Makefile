# Lower-level Makefile required by the alpha source-package rubric.
# The top-level Makefile owns the build; these targets delegate to it.

.PHONY: all test test-gui clean tar

all:
	$(MAKE) -C ..

test:
	$(MAKE) -C .. test

test-gui:
	$(MAKE) -C .. test-gui

clean:
	$(MAKE) -C .. clean

tar:
	$(MAKE) -C .. tar
