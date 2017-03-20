# Building for OS X/macOS
Please see also README.md, these are only OS X specific instructions. Especially note the remarks on running the server.

For compiling pEp Engine Server Adapter and its dependencies, make sure you have the LANG variable set.

```
export LANG=en_US.UTF-8
```

## Dependencies

### MacPorts
[Install MacPorts](https://www.macports.org/install.php) for your version of OS X/macOS.

If MacPorts is already installed on your machine, but was installed by a different user, make sure your `PATH` variable is set as follows in `~/.profile`:

```
export PATH="/opt/local/bin:/opt/local/sbin:$PATH"
```

Install dependencies packaged with MacPorts as follows.

```
sudo port install openssl boost ossp-uuid
```

### Other Dependencies

#### libevent

```
cd libevent-2.0.22-stable
export LDFLAGS=-L/opt/local
export CFLAGS=-I/opt/local/include

./configure --prefix "$HOME"

make
make install
```

## Building pEp JSON Server Adapter

```
cd server
make
```

# Running pEp JSON Server Adapter

```
./pep-json-server
```

# Testing pEp JSON Server Adapter

```
./servertest
```
