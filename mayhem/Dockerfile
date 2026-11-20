# syntax=docker/dockerfile:1.7
# Commit image for ddnet (C++20 + a small Rust cxx bridge, CMake/Ninja).
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# Build deps for CLIENT=OFF SERVER=ON TOOLS=ON: cmake/ninja, python3 (code generation), the Rust
# toolchain (CMake requires cargo for the engine's cxx bridge), libpng/zlib/sqlite/curl/ssl, gtest
# for upstream's testrunner.
USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
      cmake ninja-build python3 cargo rustc libcurl4-openssl-dev libsqlite3-dev libssl-dev \
      libpng-dev zlib1g-dev libgtest-dev libgmock-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*
# Cargo registry cache at a fixed, $HOME-independent path, writable by the build identity.
ENV CARGO_HOME=/opt/toolchains/rust/cargo
RUN mkdir -p /opt/toolchains/rust/cargo && chown -R 2000:2000 /opt/toolchains/rust

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

# Pre-fetch every crate in Cargo.lock into $CARGO_HOME (online, once) so build.sh's
# cargo build resolves offline (build.sh sets CARGO_NET_OFFLINE=true).
RUN cargo fetch --locked --manifest-path /mayhem/Cargo.toml

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
