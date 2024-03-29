all: build/build.ninja
# Redirect everything to stderr so that QtCreator sees the error messages
	ninja -C $(<D) 1>&2

.PHONY: all clean test debug install

test install: build/build.ninja
	ninja -C $(<D) $@

clean:
	rm -rf build

%/build.ninja:
	meson setup $(@D) $(MESON_OPTS)

debug: MESON_OPTS=--buildtype=debug -Db_sanitize=address,undefined
debug: build/build.ninja


.PHONY: boardproxy.includes
boardproxy.includes:
	echo . > $@
	echo build >> $@
	nix-shell --run 'echo $$NIX_CFLAGS_COMPILE| sed -Ee "s/(^| )-(I|isystem )/\n/g" | sort -u' >> $@
