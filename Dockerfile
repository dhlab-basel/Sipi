FROM gcc:5.3

MAINTAINER Ivan Subotic <ivan.subotic@unibas.ch>

RUN apt-get update && \
    apt-get -y upgrade

RUN apt-get -y install cmake && \
    apt-get -y install git && \
    apt-get -y install openjdk-7-jdk && \
    apt-get -y install libreadline-dev && \
    apt-get -y install liblog4cpp5-dev && \
    apt-get -y install gettext && \
    apt-get -y install unzip

#COPY . /usr/src/sipi
#WORKDIR /usr/src/sipi
#RUN cd build && cmake .. && make
CMD ["/bin/bash"]