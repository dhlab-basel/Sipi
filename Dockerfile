FROM dhlabbasel/sipi-base:latest

MAINTAINER Ivan Subotic <ivan.subotic@unibas.ch>

# Add everything to image.
COPY . /sipi

# Install and clean-up SIPI.
RUN cd /sipi/build && \
    make install && \
    mkdir /sipi/images && \
    mkdir /sipi/cache && \
    rm -rf /sipi/vendor && \
    rm -rf /sipi/build && \
    rm -rf /sipi/extsrcs

EXPOSE 1024

CMD ["/sipi/local/bin/sipi", "--config=/sipi/config/sipi.knora-test-docker-config.lua"]