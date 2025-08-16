# ===== Top-level recursive Makefile =====
# Runs targets in each subdirectory named Q_* (Q_1_to_4, Q_7, Q_9, ...)

# Find Q_* directories in this folder
SUBDIRS := $(shell find . -maxdepth 1 -type d -name 'Q_*' -printf '%P\n' | sort)

# Avoid noisy "Entering/Leaving directory" messages
MAKEFLAGS += --no-print-directory

.PHONY: help list all clean rebuild \
        cover_server_all cover_client_all \
        valgrind_server_all valgrind_client_all \
        helgrind_server_all helgrind_client_all \
        callgrind_server_all callgrind_client_all

help:
	@echo "Top-level targets:"
	@echo "  all                 - run 'make all' in each Q_*"
	@echo "  clean               - run 'make clean' in each Q_*"
	@echo "  rebuild             - clean then all in each Q_*"
	@echo "  cover_server_all    - run 'make cover_server' where present"
	@echo "  cover_client_all    - run 'make cover_client' where present"
	@echo "  valgrind_server_all - run 'make valgrind_server' where present"
	@echo "  valgrind_client_all - run 'make valgrind_client' where present"
	@echo "  helgrind_server_all - run 'make helgrind_server' where present"
	@echo "  helgrind_client_all - run 'make helgrind_client' where present"
	@echo "  callgrind_server_all- run 'make callgrind_server' where present"
	@echo "  callgrind_client_all- run 'make callgrind_client' where present"
	@echo ""
	@echo "Detected subdirectories:"
	@printf '  - %s\n' $(SUBDIRS)

list:
	@printf '%s\n' $(SUBDIRS)

# ---------- Basic aggregate targets ----------
all:
	@set -e; for d in $(SUBDIRS); do \
	  echo "==> $$d: all"; \
	  $(MAKE) -C $$d all; \
	done

clean:
	@set -e; for d in $(SUBDIRS); do \
	  echo "==> $$d: clean"; \
	  $(MAKE) -C $$d clean; \
	done

rebuild: clean all

# ---------- Helper to run a target only if it exists ----------
# Uses 'make -q' to detect whether a target exists in a subdir:
#  - returns 0 or 1 if target exists (up-to-date or needs build)
#  - returns 2 if target is unknown/error => we skip
define RUN_IF_PRESENT
	@set -e; for d in $(SUBDIRS); do \
	  if [ -f $$d/Makefile ]; then \
	    $(MAKE) -C $$d -q $(1) >/dev/null 2>&1; status=$$?; \
	    if [ $$status -eq 0 ] || [ $$status -eq 1 ]; then \
	      echo "==> $$d: $(1)"; \
	      $(MAKE) -C $$d $(1); \
	    else \
	      echo "--  $$d: (skipping '$(1)' - not defined)"; \
	    fi; \
	  fi; \
	done
endef

# ---------- Aggregated optional targets (only where present) ----------
cover_server_all:
	$(call RUN_IF_PRESENT,cover_server)

cover_client_all:
	$(call RUN_IF_PRESENT,cover_client)

valgrind_server_all:
	$(call RUN_IF_PRESENT,valgrind_server)

valgrind_client_all:
	$(call RUN_IF_PRESENT,valgrind_client)

helgrind_server_all:
	$(call RUN_IF_PRESENT,helgrind_server)

helgrind_client_all:
	$(call RUN_IF_PRESENT,helgrind_client)

callgrind_server_all:
	$(call RUN_IF_PRESENT,callgrind_server)

callgrind_client_all:
	$(call RUN_IF_PRESENT,callgrind_client)
