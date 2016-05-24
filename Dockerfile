FROM subotic/sipi-base:14.04

MAINTAINER Ivan Subotic <ivan.subotic@unibas.ch>

COPY . /usr/src/sipi

WORKDIR /usr/src/sipi

#RUN cd build && cmake .. && make

CMD ["/bin/bash"]