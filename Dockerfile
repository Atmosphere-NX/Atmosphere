FROM devkitpro/devkita64:20260219

ARG PYTHON_VERSION=2.7.18
ARG PYTHON_SHA256=da3080e3b488f648a3d7a4560ddee895284c3380b11d6de75edb986526b9a814
ARG PYTHON_LZ4_VERSION=2.2.1
ARG PYTHON_LZ4_SHA256=9f529a443f9eb1b49d040f0791bb3934c64b9a927609a30319c3b82f93d9525a

RUN apt update && \
    apt install automake build-essential ca-certificates wget libbz2-dev libffi-dev libgdbm-dev liblz4-dev libncurses5-dev libreadline-dev libsqlite3-dev libssl-dev liblzma-dev sudo tk-dev uuid-dev zlib1g-dev python3-lz4 python3-pip -y && \
    cd /tmp && \
    wget -q https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz && \
    echo "${PYTHON_SHA256}  Python-${PYTHON_VERSION}.tgz" | sha256sum --check && \
    tar -xzf Python-${PYTHON_VERSION}.tgz && \
    cd /tmp/Python-${PYTHON_VERSION} && \
    ./configure --prefix=/usr/local --with-ensurepip=install && \
    make -j"$(nproc)" altinstall && \
    cd /tmp && \
    wget -q https://files.pythonhosted.org/packages/86/4f/028a9ff9cfc73f317c22dc3b8f29a7c5c94098d4da590405e6b78af00e57/lz4-${PYTHON_LZ4_VERSION}-cp27-cp27m-manylinux1_x86_64.whl && \
    echo "${PYTHON_LZ4_SHA256}  lz4-${PYTHON_LZ4_VERSION}-cp27-cp27m-manylinux1_x86_64.whl" | sha256sum --check && \
    /usr/local/bin/python2.7 -m pip install --no-deps --no-index lz4-${PYTHON_LZ4_VERSION}-cp27-cp27m-manylinux1_x86_64.whl && \
    ln -fs /usr/local/bin/python2.7 /usr/local/bin/python2 && \
    ln -fs /usr/local/bin/python2.7 /usr/local/bin/python && \
    rm -rf /tmp/Python-${PYTHON_VERSION} /tmp/Python-${PYTHON_VERSION}.tgz /tmp/lz4-${PYTHON_LZ4_VERSION}-cp27-cp27m-manylinux1_x86_64.whl && \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/* /usr/share/doc /usr/share/man
    
# Install latest hactool & switch-tools from dkp pacman & update everything
RUN dkp-pacman -S switch-tools hactool --noconfirm && dkp-pacman -Syu --noconfirm

# # Install hactool from Git (optional)
# ENV HACTOOL_REV=1d64a83450e025622f3468c28fc4164dad2c5ef6
# RUN cd /tmp && \
#     mkdir -p /tmp/hactool && \
#     cd /tmp/hactool && \
#     git init && \
#     git remote add origin https://github.com/SciresM/hactool.git && \
#     git fetch --depth 1 origin $HACTOOL_REV && \
#     git reset --hard FETCH_HEAD && \
#     cp config.mk.template config.mk && \
#     make -j$(nproc) && \
#     cp hactool /opt/devkitpro/tools/bin/hactool && \
#     cd /tmp && \
#     rm -rf /tmp/hactool

# # Install switch-tools from Git (optional)
# ENV SWITCH_TOOLS_REV=22756068dd0ed6ff9734c59cb4f99ebd3f62555b
# RUN cd /tmp && \
#     mkdir -p /tmp/switch-tools && \
#     cd /tmp/switch-tools && \
#     git init && \
#     git remote add origin https://github.com/switchbrew/switch-tools.git && \
#     git fetch --depth 1 origin $SWITCH_TOOLS_REV && \
#     git reset --hard FETCH_HEAD && \
#     ./autogen.sh && ./configure && make -j$(nproc) && \
#     make install prefix=/opt/devkitpro/tools && \
#     cd /tmp && \
#     rm -rf /tmp/switch-tools

# Install libnx from Git (optional)
# ENV LIBNX_REV=dbcc1beafc6b47b5ffbeb8ba82463a7d45da40bb
# RUN cd /tmp && \
#     mkdir -p /tmp/libnx && \
#     cd /tmp/libnx && \
#     git init && \
#     git remote add origin https://github.com/switchbrew/libnx.git && \
#     git fetch --depth 1 origin $LIBNX_REV && \
#     git reset --hard FETCH_HEAD && \
#     make -j$(nproc) && \
#     mkdir -p /opt/devkitpro/libnx && \
#     cd nx && \
#     make DESTDIR=/opt/devkitpro/libnx install && \
#     cd /tmp && \
#     rm -rf /tmp/libnx

RUN useradd -m atmosphere && \
    echo "atmosphere ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/atmosphere && \
    chmod 0440 /etc/sudoers.d/atmosphere && \
    echo "atmosphere:atmosphere" | chpasswd && \
    usermod -aG sudo atmosphere

USER atmosphere
CMD ["/bin/bash"]
