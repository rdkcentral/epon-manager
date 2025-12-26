# Makefile.am for tests directory

if ENABLE_TESTS

SUBDIRS = hal_mock unit integration

# Run all tests
check-local:
	@echo "====================================="
	@echo "Running EPON Manager Test Suite"
	@echo "====================================="
	@$(MAKE) -C unit check
	@$(MAKE) -C integration check
	@echo "====================================="
	@echo "All tests completed"
	@echo "====================================="

endif
