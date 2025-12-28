# Top-level Makefile.am for EPON Manager

ACLOCAL_AMFLAGS = -I m4

# Subdirectories to build
if ENABLE_TESTS
SUBDIRS = include tests src
else
SUBDIRS = include src
endif

# Extra files to include in distribution
EXTRA_DIST = \
    README.md \
    DIRECTORY_STRUCTURE.md \
    EPON_HAL_Proposal.md \
    design_docs \
    docs \
    scripts/build_and_test.sh \
    scripts/setup_workspace.sh \
    epon_hal.h

# Create lib directory at build time
all-local:
	$(MKDIR_P) $(builddir)/lib

# Clean build artifacts
clean-local:
	-rm -rf $(builddir)/lib/*.so
	-rm -rf $(builddir)/lib/*.a

# Installation hooks
install-data-hook:
	$(MKDIR_P) $(DESTDIR)$(sysconfdir)/epon

# Uninstall hooks
uninstall-local:
	-rmdir $(DESTDIR)$(sysconfdir)/epon

.PHONY: test
test:
	@if test "x$(ENABLE_TESTS)" = "xyes"; then \
		$(MAKE) -C tests check; \
	else \
		echo "Tests not enabled. Configure with --enable-tests"; \
	fi
