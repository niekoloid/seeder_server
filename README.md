## Prerequisite
* Ubuntu 16.04 / 18.04
* Install dependencies
```bash
$ sudo apt-get update
$ sudo apt-get install libboost-system-dev
```
---
## Build
```bash
$ ./build.sh
```
`server` and `client` object files are to be built.
---
## Run
* You need more than two terminals to test this program.
* Please try running multiple clients from multiple terminals.

### terminal 1

```bash
$ ./server
```

### terminal 2
```bash
$ ./client
```

### terminal 3
```bash
$ ./client
```
