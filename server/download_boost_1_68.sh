mkdir -p contrib ; cd contrib
wget -c https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2
mkdir -p boost
tar --strip-components=1 --wildcards -xvf boost_1_68_0.tar.bz2 \
  -C ./boost boost_1_68_0/boost/{asio,process,winapi}\*
