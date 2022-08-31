FROM ubuntu:latest
RUN apt update && apt install -y \
    gcc \
    'g++' \
    cmake \
    nasm \
    git \
    zlib1g-dev \
    libexpat1-dev \
    pkgconf \
    clang \
    autoconf \
    automake \
    autotools-dev \
    libtool \
    xutils-dev \
    libssl-dev \
    ca-certificates \
    curl \
    file \
    sudo \
    python-is-python3 \
    python3 \
    python3-dev \
    python3-pip
RUN cd ~ && git clone --depth 1 'https://code.videolan.org/videolan/x264.git' && cd x264 \
    && ./configure --disable-cli --enable-strip --enable-pic --enable-shared --disable-static --prefix=/usr \
    && make -j$(grep -c ^processor /proc/cpuinfo) && make install \
    && cd ~ && rm -rf x264
RUN cd ~ && git clone --depth 1 'https://git.ffmpeg.org/ffmpeg.git' && cd ffmpeg \
    && ./configure --enable-shared --disable-static --enable-gpl --enable-version3 --enable-libx264 --prefix=/usr \
    && make -j$(grep -c ^processor /proc/cpuinfo) && make install \
    && cd ~ && rm -rf ffmpeg
RUN pip install Cython
# Enable GPG sign by default
RUN git config --global commit.gpgsign true
RUN useradd -ms /bin/bash life && \
    usermod -aG sudo life && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
USER life
