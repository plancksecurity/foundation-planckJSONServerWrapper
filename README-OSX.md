# Notes for OS X

Please see also the README.md, these are only OS X specific instructions.
You'll need some special environment to run the server, see "Running the server".

# Building libevent on OS X

## MacPorts

Install [MacPorts](https://www.macports.org/) for your
[version of OS X/macOS](https://www.macports.org/install.php).

Note that you need [Xcode installed](https://www.macports.org/install.php)
for MacPorts, and for building the engine. You also need to accept Xcode's EULA.

```
sudo port install openssl
sudo port install boost
sudo port install ossp-uuid
```

## libevent

```
export LDFLAGS=-L/opt/local
export CFLAGS=-I/opt/local/include

./configure --prefix $HOME

make
make install
```

## server/Makefile

```
cd server
make
```

# Running the server

```
LD_LIBRARY_PATH=/opt/local/lib ./mt-server
```
