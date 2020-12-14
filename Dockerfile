# STAGE 1: Build
FROM dhlabbasel/sipi-base:18.04 as builder

WORKDIR /sipi

# Add everything to image.
COPY . .

# Build SIPI.
RUN mkdir -p /sipi/build-linux && \
    cd /sipi/build-linux && \
    cmake -DMAKE_DEBUG:BOOL=OFF .. && \
    make

# STAGE 2: Setup
FROM dhlabbasel/sipi-base:18.04

MAINTAINER Ivan Subotic <400790+subotic@users.noreply.github.com>

WORKDIR /sipi

EXPOSE 1024

RUN mkdir -p /sipi/images/knora && \
    mkdir -p /sipi/cache

# Copy Sipi binary from build stage
COPY --from=builder /sipi/build-linux/sipi /sipi/sipi

ENTRYPOINT [ "/sipi/sipi" ]

CMD ["--config=/sipi/config/sipi.config.lua"]
