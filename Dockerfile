# first stage does the building
FROM dhlabbasel/sipi-base:18.04 as build-stage

MAINTAINER Ivan Subotic <ivan.subotic@unibas.ch>

# Add everything to image.
COPY . /sipi

# Install and clean-up SIPI.
RUN cd /sipi/build && \
    cmake .. && \
    make && \
    make install && \
    mkdir -p /sipi/images/knora && \
    mkdir -p /sipi/cache && \
    rm -rf /sipi/vendor && \
    rm -rf /sipi/build && \
    rm -rf /sipi/extsrcs


# starting second stage
FROM ubuntu:18.04

RUN \
    mkdir -p /sipi/images/knora && \
    mkdir -p /sipi/cache

# copy the binary from the `build-stage`
COPY --from=build-stage /sipi/local /sipi/local
COPY --from=build-stage /sipi/scripts /sipi/scripts
COPY --from=build-stage /sipi/config /sipi/config
COPY --from=build-stage /sipi/server /sipi/server
COPY --from=build-stage /sipi/certificate /sipi/certificate
# COPY --from=build-stage /usr/bin/lib /usr/bin/lib
# COPY --from=build-stage /usr/bin/include /usr/bin/include
COPY --from=build-stage /usr/local/lib /usr/local/lib
COPY --from=build-stage /usr/local/include /usr/local/include

EXPOSE 1024

WORKDIR /sipi

ENTRYPOINT [ "/sipi/local/bin/sipi" ]

CMD ["--config=/sipi/config/sipi.config.lua"]