FROM dhlabbasel/sipi-base:18.04

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

EXPOSE 1024

ENTRYPOINT [ "/sipi/local/bin/sipi" ]

CMD ["--config=/sipi/config/sipi.config.lua"]