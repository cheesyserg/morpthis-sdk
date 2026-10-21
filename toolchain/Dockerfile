# The pinned Morpthis cross toolchain.
#
# Build with the URL of a published tarball:
#
#   docker build \
#     --build-arg MORPTHIS_TOOLCHAIN_URL=https://example.invalid/morpthis-toolchain.tar.xz \
#     [--build-arg MORPTHIS_TOOLCHAIN_SHA256=...] \
#     -t morpthis-sdk-toolchain toolchain/
#
# Then build an app with:
#
#   docker run --rm -v "$PWD:/work" -w /work morpthis-sdk-toolchain \
#     make -C apps/calculator MORPTHIS_MODE=device check-abi

FROM ubuntu:22.04

ARG MORPTHIS_TOOLCHAIN_URL
ARG MORPTHIS_TOOLCHAIN_SHA256

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH=/opt/morpthis-toolchain/bin:$PATH

RUN apt-get update && apt-get install -y --no-install-recommends \
		build-essential \
		ca-certificates \
		curl \
		git \
		make \
		python3 \
		xz-utils \
	&& rm -rf /var/lib/apt/lists/*

COPY fetch.sh /usr/local/bin/fetch-morpthis-toolchain
RUN chmod +x /usr/local/bin/fetch-morpthis-toolchain \
	&& MORPTHIS_TOOLCHAIN_URL="${MORPTHIS_TOOLCHAIN_URL}" \
	   MORPTHIS_TOOLCHAIN_SHA256="${MORPTHIS_TOOLCHAIN_SHA256}" \
	   fetch-morpthis-toolchain /opt/morpthis-toolchain

WORKDIR /work
