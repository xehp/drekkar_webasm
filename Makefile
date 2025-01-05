# https://www.gnu.org/software/make/manual/html_node/Recursion.html
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
# https://gist.github.com/linse/87f3b44d59a9d298309c

SUBDIRS = test_code drekkar_webasm_runtime

.PHONY: all clean $(SUBDIRS)

# subdirs: $(SUBDIRS)

all clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f Makefile $@; \
	done

$(SUBDIRS):
	$(MAKE) -C $@

