#!/usr/bin/env bash
# mayhem/build.sh — ddnet: sanitized engine/game object libraries + libFuzzer harnesses, and
# upstream's gtest testrunner with normal flags.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS
# Every crate in Cargo.lock was fetched into $CARGO_HOME by the Dockerfile; never hit the network.
export CARGO_HOME="${CARGO_HOME:-/opt/toolchains/rust/cargo}" CARGO_NET_OFFLINE=true

cd "$SRC"
COMMON_CMAKE=(-G Ninja -DCLIENT=OFF -DSERVER=ON -DTOOLS=ON -DVIDEORECORDER=OFF -DDOWNLOAD_GTEST=OFF
  -DPRECOMPILE_HEADERS=OFF -DSECURITY_COMPILER_FLAGS=OFF -DPREFER_BUNDLED_LIBS=OFF -DWEBSOCKETS=OFF
  -DAUTOUPDATE=OFF -DDISCORD=OFF -DSTEAM=OFF)

# 1) Sanitized project build (Debug: dbg_assert + _GLIBCXX_DEBUG on). Build dirs are /build-* (gitignored).
# SanitizerCoverage on the project objects too (not only the harness), so libFuzzer sees edges.
COV_FLAGS="-fsanitize=fuzzer-no-link"
B=build-fuzz
cmake -S . -B "$B" "${COMMON_CMAKE[@]}" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="$SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS" -DCMAKE_CXX_FLAGS="$SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS"
# map_test + dilate pull in every object library the harnesses link (engine-shared, game-shared,
# engine-gfx, json, md5) and the Rust bridge static lib.
cmake --build "$B" -j"$MAYHEM_JOBS" --target map_test dilate game-server

OBJS=()
for lib in engine-shared game-shared engine-gfx json md5; do
  while IFS= read -r o; do OBJS+=("$o"); done < <(find "$B/CMakeFiles/$lib.dir" -name '*.o' | LC_ALL=C sort)
done
# Server-side game objects (CSaveTeam etc.) for the harnesses that need them.
SERVER_OBJS=()
for lib in game-server-without-main rust-bridge-shared; do
  while IFS= read -r o; do SERVER_OBJS+=("$o"); done < <(find "$B/CMakeFiles/$lib.dir" -name '*.o' | LC_ALL=C sort)
done
RUSTLIB="$B/debug/libddnet_engine_shared.a"
[ -f "$RUSTLIB" ] || { echo "missing $RUSTLIB" >&2; exit 1; }
LIBS=("$RUSTLIB" -lcrypto -lcurl -lsqlite3 -lpng -lz -lrt -ldl -lpthread)
CXXF=(-std=c++20 -fno-exceptions -fsigned-char -DCONF_DEBUG -DCONF_OPENSSL -D_FILE_OFFSET_BITS=64
  -D_GLIBCXX_DEBUG -I"$B/src" -Isrc -Isrc/rust-bridge -fuse-ld=lld)

$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$B/lsan_off.o"

HARNESSES=(map collision packet netmsg snapshot png huffman stun serverinfo console map_resave demo savestring)
for h in "${HARNESSES[@]}"; do
  EXTRA=()
  [ "$h" = savestring ] && EXTRA=("${SERVER_OBJS[@]}")
  $CXX $SANITIZER_FLAGS $COV_FLAGS $DEBUG_FLAGS "${CXXF[@]}" -c "mayhem/harnesses/fuzz_$h.cpp" -o "$B/fuzz_$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "${CXXF[@]}" "$B/fuzz_$h.o" "$B/lsan_off.o" \
    "${EXTRA[@]}" "${OBJS[@]}" "${LIBS[@]}" -o "/mayhem/fuzz_$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "${CXXF[@]}" $COV_FLAGS "$B/standalone_main.o" "$B/fuzz_$h.o" "$B/lsan_off.o" \
    "${EXTRA[@]}" "${OBJS[@]}" "${LIBS[@]}" -o "/mayhem/fuzz_$h-standalone"
done

# 2) Upstream's gtest testrunner with the project's normal flags (independent build tree).
T=build-tests
cmake -S . -B "$T" "${COMMON_CMAKE[@]}" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build "$T" -j"$MAYHEM_JOBS" --target testrunner
