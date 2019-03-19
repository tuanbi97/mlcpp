FROM workspace:1.0

VOLUME /detector

RUN apt-get update && \
	apt-get install -y software-properties-common && \
	add-apt-repository ppa:ubuntu-toolchain-r/test && \
	apt-get update && \
	apt-get install -y gcc-7 g++-7 && \
	apt-get install -y build-essential && \
	apt-get install -y cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev && \

	cd ~ && \
	git clone https://github.com/opencv/opencv.git && \
	cd opencv && \
	git checkout 3.4 && \
	cd .. && \
	git clone https://github.com/opencv/opencv_contrib.git && \
	cd opencv_contrib && \
	git checkout 3.4 && \
	cd .. && \

	cd opencv && \
	mkdir build && \
	cd build && \

	cmake -DCMAKE_BUILD_TYPE=Release -OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib -DCMAKE_INSTALL_PREFIX=/usr/local .. && \

	make -j4 && \

	make install && \
	cd ~ && \
	printf 'Y\n' | apt remove cmake && \
	wget https://github.com/Kitware/CMake/releases/download/v3.13.4/cmake-3.13.4-Linux-x86_64.sh && \
	chmod +x cmake-3.13.4-Linux-x86_64.sh && \
	printf 'y\nY\n' | ./cmake-3.13.4-Linux-x86_64.sh && \
	cp cmake-3.13.4-Linux-x86_64/bin/* /usr/local/bin && \
	cp cmake-3.13.4-Linux-x86_64/bin/* /usr/bin && \
	cp -r cmake-3.13.4-Linux-x86_64/share/* /usr/local/share
	

	
