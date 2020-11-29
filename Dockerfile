FROM ubuntu:latest

ENV DEBIAN_FRONTEND="noninteractive"
RUN apt-get update && apt-get upgrade -y

RUN apt install -y \
	binutils \
	build-essential \
	cc65 \
	ccache \
	clang \
	clang-format \
	clang-tidy \
	cmake \
	cppcheck \
	curl \
	doxygen \
	gcovr \
	gdb \
	git \
	graphviz \
	iwyu \
	kcachegrind \
	lcov \
	libsdl2-dev \
	lldb \
	llvm \
	python3 \
	python3-pip \
	python3-setuptools \
	silversearcher-ag \
	tar \
	unzip \
	valgrind \
	vim 

RUN echo "Installing dependencies not found in the package repos..."

RUN pip3 install Jinja2 Pygments cmake_format==0.6.11 pyyaml