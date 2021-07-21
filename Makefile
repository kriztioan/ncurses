PROJECTS:=$(wildcard */.)

all clean: $(PROJECTS)

.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
