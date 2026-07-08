FROM ubuntu:18.04 as build

RUN apt-get update && \
    apt-get install -y \
        bsdmainutils \
        build-essential \
        python3 \
        unzip \
        wget

RUN wget https://github.com/andrewwutw/build-djgpp/releases/download/v3.0/djgpp-linux64-gcc930.tar.bz2 && \
  echo '82c5946fc3155face1299e72d7a07c5fa47142942bd3074d596714a346932e9e  djgpp-linux64-gcc930.tar.bz2' | sha256sum djgpp-linux64-gcc930.tar.bz2 && \
  tar xjf djgpp-linux64-gcc930.tar.bz2 && \
  rm djgpp-linux64-gcc930.tar.bz2

RUN mkdir /sm64
WORKDIR /sm64
ENV PATH="/sm64/tools:/djgpp/bin:${PATH}"

CMD echo 'usage: docker run --rm --mount type=bind,source="$(pwd)",destination=/sm64 sm64 make VERSION=${VERSION:-us} -j4\n' \
         'see https://github.com/n64decomp/sm64/blob/master/README.md for advanced usage'
