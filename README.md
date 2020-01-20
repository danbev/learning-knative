## Learning Knative 
Knative looks to build on Kubernetes and present a consistent, standard pattern
for building and deploying serverless and event-driven applications. 

Knative allows services to scale down to zero and scale up from zero. 

### Background knowledge
Before we start there are a few things that I was not 100% clear about and this
section aims to sort this out to allow for better understanding of the underlying
technologies.

#### Container
A container is a process that is isolated from other processes using Linux
kernal features like cgroups, namespaces, mounted union fs (chrooted), etc. 

When a container is deployed what happens is that the above mentioned features
are configured, a filesystem mounted, and a process is started. The metadata and
the filesystem is contained in an image (more on this later).

A container image, it has all the libraries, file it needs to run. It does not
have an entire OS but instead uses the underlying hosts kernel which saves space
compared to a separate VM. 

Also it is worth mentioning that a running container is a process (think unix process)
which has a separate control group (cgroup), and namespace (mnt, IPC, net, usr, pid,
and uts (Unix Time Share system)). It could also include seccomp (Secure Computing mode)
which is a way to filter the system calls allowed to be performed, apparmor (prevents
access to files the process should not access), and linux capabilities (reducing
what a privileged process can do). More on these three security features can be found later in this
document.

### Namespaces
The namespace API consists of three system calls:
* clone
* unshare
* setns

A namespace can be created using `clone`:
```c
int clone(int (*child_func)(void *), void *child_stack, int flags, void *arg);
```
The `child_func` is a function pointer to the function that the new child process
will execute, and `arg` are the arguments that that function might take. Linux
also has the `fork` system call which also creates a child
process, but clone allows control over the things that get shared between the
parent and the child process. Things like if they should share the virtual
address space, the file descriptor table, the signal handler table, and also
allows the new process to be placed in separate namespaces. This is controlled
by the `flags` parameter. This is also how threads are created on Linux and
the kernel has the same internal representation for this which is the `task_struct`

`child_stack` specifies the location of the stack used by the child process.

There is an example of the `clone` systemcall in [clone.c](./clone.c) which
can be compiled and run using the following commands:
```console
$ docker run -ti --privileged -v$PWD:/root/src -w /root/src gcc
$ gcc -o clone clone.c
$ ./clone
parent pid: 81
child hostname: child_host
child pid: 1
child ppid: 0
parent hostname: caa66b227dfe
```
The goal of this is just to give an example and show the names of the flags that
control the namespaces. 
`

##### cgroups
cgroups allows the Linux OS to manage and monitor resources allocated to a process
and also set limits for things like CPU, memory, network. This is so that one
process is not allowed to hog all the resources and affect others. 

##### secccomp (Secure Computing)
Is a Linux kernel feature that restricts the system calls a process can call.
So if someone was to gain access they would not be able to use any other system
call than the ones that were specified.

The command that controls this is named `prctl` (process control). There is an
example of using `prctl` in [seccomp.c](./seccomp.c):
```console
$ docker run -ti --privileged -v$PWD:/root/src -w /root/src gcc
$ gcc -o seccomp seccomp.c
./seccomp
pid: 351
setting restrictions...
running with restrictions. Allowed system calls areread(), write(), exit()
try calling getpid()
Killed
```
We can run this with strace to see the system calls being made:
```console
$ apt-get update
$ apt-get install strace
root@d978e6c92dca:~/src# strace ./seccomp
execve("./seccomp", ["./seccomp"], 0x7fff025d34c0 /* 10 vars */) = 0
brk(NULL)                               = 0x1f2d000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=37087, ...}) = 0
mmap(NULL, 37087, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f1985142000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\260A\2\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0755, st_size=1824496, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f1985140000
mmap(NULL, 1837056, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f1984f7f000
mprotect(0x7f1984fa1000, 1658880, PROT_NONE) = 0
mmap(0x7f1984fa1000, 1343488, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x22000) = 0x7f1984fa1000
mmap(0x7f19850e9000, 311296, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x16a000) = 0x7f19850e9000
mmap(0x7f1985136000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b6000) = 0x7f1985136000
mmap(0x7f198513c000, 14336, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f198513c000
close(3)                                = 0
arch_prctl(ARCH_SET_FS, 0x7f1985141500) = 0
mprotect(0x7f1985136000, 16384, PROT_READ) = 0
mprotect(0x403000, 4096, PROT_READ)     = 0
mprotect(0x7f1985173000, 4096, PROT_READ) = 0
munmap(0x7f1985142000, 37087)           = 0
getpid()                                = 350
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0), ...}) = 0
brk(NULL)                               = 0x1f2d000
brk(0x1f4e000)                          = 0x1f4e000
write(1, "pid: 350\n", 9pid: 350
)               = 9
write(1, "setting restrictions...\n", 24setting restrictions...
) = 24
prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT) = 0
write(1, "running with restrictions. Allow"..., 75running with restrictions. Allowed system calls areread(), write(), exit()
) = 75
write(1, "try calling getpid()\n", 21try calling getpid()
)  = 21
getpid()                                = ?
+++ killed by SIGKILL +++
Killed
```
In this case we were not able to specify exactly which system calls are allowed
but this can be done using Berkley Paket Filtering (BPF). [seccomp_bpf.c](./seccomp_bpf.c):
```console
$ apt-get install libseccomp-dev
$ gcc -lseccomp -o seccomp_bpf seccomp_bpf.c
```

##### namespaces
Are used to isolate process from each other. Each container will have its own
namespace but it is also possible for multiple containers to be in the same
namespace which is what the deployment unit of kubernetes is; the pod.

###### PID (CLONE_NEWPID)
In a `pid` namespace your process becomes PID 1. You can only see this process
and child processes, all others on the underlying host system are "gone".

###### UTS (CLONE_NEWUTS)
Isolates `domainname` and `hostname` allowing each container to have its own
hostname and NIS domain name. The hostnaame and domain name are retrived by
the [uname](http://man7.org/linux/man-pages/man2/uname.2.html) system call and
the struct passed into this function is named `utsname` (UNIX Time-share System)

###### IPC (CLONE_NEWIPC)
Isolate System V IPC Objects and POSIX message queues. Each namespace will have
its own set of these.


###### Network (CLONE_NEWNET)
A `net` namespace for isolating network ip/ports, IP routing tables.

The following is an example of creating a network namespace just to get a feel
for what is involved.
```console
$ docker run --privileged -ti centos /bin/bash
```
A network namespace can be created using `ip netns`:
```console
$ ip netns add something
$ ip netns list
something
```
With a namespace created we can add virtual ethernet (veth) interfaces to it. These
come in pairs and can be thought of as a cable between the namespace and the outside
world (which is usually a bridge in the kubernetes case I think). So the other end
would be connected to the bridge. Multiple namespaces can be connected to the same
bridge.

First we can create a virtual ethernet pair (veth pair) named `v0` and `v1`:
```console
$ ip link add v0 type veth peer name v1
$ ip link list
...
4: v1@v0: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 8e:3f:28:e1:e8:d9 brd ff:ff:ff:ff:ff:ff
5: v0@v1: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 1e:35:a5:17:76:6d brd ff:ff:ff:ff:ff:ff
...
```
Next, we add one end of the virtual ethernet pair to the namespace we created:
```console
$ ip link set v1 netns something
```
We also want to give `v1` and ip address and enable it:
```console
$ ip netns exec something ip address add 172.16.0.1 dev v1
$ ip netns exec something ip link set v1 up
$ ip link set dev v0 up
$ ip netns exec something ip address show dev v1
4: v1@if5: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state LOWERLAYERDOWN group default qlen 1000
    link/ether 9a:56:bf:12:32:0d brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.16.0.1/32 scope global v1
       valid_lft forever preferred_lft forever
```
We can find the ip address of eth0 in the default namespace using:
```console
$ ip address show dev eth0
86: eth0@if87: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default
    link/ether 02:42:ac:11:00:02 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 172.17.0.2/16 brd 172.17.255.255 scope global eth0
       valid_lft forever preferred_lft forever
```
So can we ping that address from our `something` namespace?
```console
$ ip netns exec something ping 172.17.0.2
connect: Network is unreachable
```
No, we can't because there is no routing table for the namespace:
```console
$ ip netns exec something ip route list
```
We should be able to add a default route that sends anything not on host to `v1`:
```console
$ ip netns exec something ip route add default via 172.16.0.1 dev v1
```
We also need to add a route for this container in the host so that the return
packet can be routed back:
```console
$ ip route add 172.16.0.1/32 dev v0
$ ip netns exec something ip link set lo up
```
With that in place we should be able to ping:
```console
$ ip netns exec something ping 172.17.0.2
ip netns exec something ping 172.17.0.2
PING 172.17.0.2 (172.17.0.2) 56(84) bytes of data.
64 bytes from 172.17.0.2: icmp_seq=1 ttl=64 time=0.052 ms
64 bytes from 172.17.0.2: icmp_seq=2 ttl=64 time=0.079 ms
...
```
Notice that we have only added a namespace and not started a process/container. It is
in fact the kernel networking stack that is replying to this ping.

This would look something like the following:
```
     +--------------------------------------------------------+
     |     Default namespace                                  |
     | +---------------------------------------------------+  |
     | |  something namespace                              |  |
     | | +--------------+  +-----------------------------+ |  |
     | | | v1:172.16.0.1|  | routing table               | |  |
     | + +--------------+  |default via 172.16.0.1 dev v1| |  |
     | |   |               +-----------------------------+ |  |
     | +---|-----------------------------------------------+  |
     |     |                                                  |
     |   +----+                                               |
     |   | v0 |                                               |
     |   +----+                                               |
     |                                                        |
     |   +----+            +------------------------------+   |
     |   |eth0|            | routing table                |   |
     |   +----+            |172.16.0.1 dev v0 scope link  |   |
     |                     +------------------------------+   |
     +--------------------------------------------------------+
```

So we have see how we can have a single namespace on a host. If we want to add
more namespaces, those namespaces not only have to be able to connect with the
host but also with each other.

Lets start by adding a second namespace:
```console
$ ip link add v2 type veth peer name v3
$ ip netns add something2
$ ip link set v3 netns something2
$ ip netns exec something2 ip address add 172.16.0.2 dev v3
$ ip netns exec something2 ip link set v3 up
$ ip netns exec something2 ip link set lo up
$ ip link set dev v2 up
$ ip netns exec something2 ip route add default via 172.16.0.2 dev v3
```

```console
$ ip link add bridge0 type bridge
$ ip link set dev v0 master bridge0
$ ip link set dev v2 master bridge0
$ ip address add 172.168.0.3/24 dev bridge0
$ ip link dev bridge 0 up
```

We can verify that we can ping from the `something` namesspace to `something2`:
```console
$ ip netns exec something ping 172.16.0.2
PING 172.16.0.2 (172.16.0.2) 56(84) bytes of data.
64 bytes from 172.16.0.2: icmp_seq=1 ttl=64 time=0.338 ms

$ ip netns exec something2 ping 172.16.0.1
PING 172.16.0.1 (172.16.0.1) 56(84) bytes of data.
64 bytes from 172.16.0.1: icmp_seq=1 ttl=64 time=0.061 ms
```
But can we ping the second container from the host?
```console
$ ping 172.16.0.2
PING 172.16.0.2 (172.16.0.2) 56(84) bytes of data.
...
```
For this to work we need a route in the host:
```console
$ ip route add 172.16.0.0/24 dev bridge0
```

After having done this our configuration should look something like this:
```
     +-----------------------------------------------------------------------------------------------------------+
     |     Default namespace                                                                                     |
     | +---------------------------------------------------+ +-------------------------------------------------+ |
     | |  something namespace                              | | something2 namespace                            | |
     | | +--------------+  +-----------------------------+ | | +-------------+ +-----------------------------+ | |
     | | | v1:172.16.0.1|  | routing table               | | | |v3:172.16.0.2| | routing table               | | |
     | + +--------------+  |default via 172.16.0.1 dev v1| | | +-------------+ |default via 172.16.0.2 dev v3| | |
     | |   |               +-----------------------------+ | |    |            +-----------------------------+ | |
     | +---|-----------------------------------------------+ +----|--------------------------------------------+ |
     |     |                                                      |                                              |
     |   +------------------------------------------------------------------------------------------------+      |
     |   | | v0 |             bridge0                           | v2 |                                    |      |
     |   | +----+           72.168.0.3                          +----+                                    |      |
     |   +------------------------------------------------------------------------------------------------+      |
     |                                                                                                           |
     |   +----+            +-------------------------------+                                                     |
     |   |eth0|            | routing table                 |                                                     |
     |   +----+            |172.16.0.0/24 dev bridge0 scope|                                                     |
     |                     +-------------------------------+                                                     |
     +-----------------------------------------------------------------------------------------------------------+
```
If you have docker deployed the bridge would be named `docker0`. For example:
```console
$ docker run -it --rm --privileged --pid=host justincormack/nsenter1
$ ip link list
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP mode DEFAULT group default qlen 1000
    link/ether 02:50:00:00:00:01 brd ff:ff:ff:ff:ff:ff
5: docker0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DEFAULT group default
    link/ether 02:42:88:76:10:4c brd ff:ff:ff:ff:ff:ff
87: veth2a62021@if86: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue master docker0 state UP mode DEFAULT group default
    link/ether 2e:70:0c:6d:61:aa brd ff:ff:ff:ff:ff:ff link-netnsid 0
89: veth24a8a43@if88: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue master docker0 state UP mode DEFAULT group default
    link/ether d2:69:da:8b:99:b6 brd ff:ff:ff:ff:ff:ff link-netnsid 1
```

###### User (CLONE_NEWUSER)
Isolates user and group IDs.




##### capabilities
A process in Linux can be either privileged or unprivileged. Capabilities allows
limiting the privileges for the superuser, so that if the program is compromised
it will not have all privileges and hopefully not be able to do as much harm. 
As an example, if you have a web server and you want it to listen to port 80 which
requires root permission. But giving the web server root permission will allow
it to to much more. Instead the binary can be given the CAP_NET_BIND_SERVICE
capability.


Are privileges that can be enabled per process(thread/task). The root user,
effective user id 0 (EUID 0) has all capabilities enabled. The Linux kernel
always checks the capabilites and does not check that the user is root (EUID 0).

You can use the following command to list the capabilities:
```console
$ capsh --print
```
```console
$ cat /proc/1/task/1/status
...
CapInh:	0000003fffffffff
CapPrm:	0000003fffffffff
CapEff:	0000003fffffffff
CapBnd:	0000003fffffffff
CapAmb:	0000000000000000
...
```
Example of using capabilities:
```console
$ docker run -ti --privileged -v$PWD:/root/src -w /root/src gcc
$ chmod u-s /bin/ping 
$ adduser danbev
$ ping localhost
ping: socket: Operation not permitted
```
We first removed the `setuid` for ping and then added a new user and verified
that they cannot use ping and get the error above.

Next, lets add the CAP_NET_RAW capability:
```console
$ setcap cap_net_raw+p /bin/ping
$ su - danbev
$ ping -c 1 localhost
PING localhost (127.0.0.1) 56(84) bytes of data.
64 bytes from localhost (127.0.0.1): icmp_seq=1 ttl=64 time=0.053 ms

--- localhost ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 0.053/0.053/0.053/0.000 ms
```

We can specify capabilities when we start docker instead of using `--privileged`
as this:
```console
$ docker run -ti --cap-add=NEW_RAW -v$PWD:/root/src -w /root/src gcc
$ su danbev
$ ping -c 1 localhost
```

#### Apparmor
Is a mandatory access control framework which uses whitelist/blacklist for
the access to objects, like file, paths etc. So this can limit what files 
a process can access for example.


The component responsible for all this work, setting the limits for cgroups, configuring
the namespaces, mounting the filesystem, and starting the process is the
responsibility of the container runtime.

### Image format

What about an docker image, what does it look like?  

We can use a tool named `skopeo` and `umoci` to inspect and find out more about
images.
```console
$ brew install skopeo
```
The image I'm using is the following:
```console
$ skopeo inspect docker://dbevenius/faas-js-example
{
    "Name": "docker.io/dbevenius/faas-js-example",
    "Digest": "sha256:69cc8b6087f355b7e4b2344587ae665c61a067ee05876acc3a5b15ca2b15e763",
    "RepoTags": [
        "0.0.3",
        "latest"
    ],
    "Created": "2019-11-25T08:37:50.894674023Z",
    "DockerVersion": "19.03.3",
    "Labels": null,
    "Architecture": "amd64",
    "Os": "linux",
    "Layers": [
        "sha256:e7c96db7181be991f19a9fb6975cdbbd73c65f4a2681348e63a141a2192a5f10",
        "sha256:95b3c812425e243848db3a3eb63e1e461f24a63fb2ec9aa61bcf5a553e280c07",
        "sha256:778b81d0468fbe956db39aca7059653428a7a15031c9483b63cb33798fcdadfa",
        "sha256:28549a15ba3eb287d204a7c67fdb84e9d7992c7af1ca3809b6d8c9e37ebc9877",
        "sha256:0bcb2f6e53a714f0095f58973932760648f1138f240c99f1750be308befd9436",
        "sha256:5a4ed7db773aa044d8c7d54860c6eff0f22aee8ee56d4badf4f890a3c82e6070",
        "sha256:aaf35efcb95f6c74dc6d2c489268bdc592ce101c990729280980da140647e63f",
        "sha256:c79d77af46518dfd4e94d3eb3a989a43f06c08f481ab3a709bc5cd5570bb0fe2"
    ],
    "Env": [
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "NODE_VERSION=12.10.0",
        "YARN_VERSION=1.17.3",
        "HOME=/home/node"
    ]
}
```

```console
$ skopeo --insecure-policy copy docker://dbevenius/faas-js-example oci:faas-js-example-oci:latest
Getting image source signatures
Copying blob e7c96db7181b done
Copying blob 95b3c812425e done
Copying blob 778b81d0468f done
Copying blob 28549a15ba3e done
Copying blob 0bcb2f6e53a7 done
Copying blob 5a4ed7db773a done
Copying blob aaf35efcb95f done
Copying blob c79d77af4651 done
Copying config c5b8673f93 done
Writing manifest to image destination
Storing signatures
```
We can take a look at the directory layout:
```console
$ ls faas-js-example-oci/
blobs		index.json	oci-layout
```
Lets take a look at index.json:
```json
$ cat index.json | python3 -m json.tool
{
    "schemaVersion": 2,
    "manifests": [
        {
            "mediaType": "application/vnd.oci.image.manifest.v1+json",
            "digest": "sha256:be5c2a500a597f725e633753796f1d06d3388cee84f9b66ffd6ede3e61544077",
            "size": 1440,
            "annotations": {
                "org.opencontainers.image.ref.name": "latest"
            }
        }
    ]
}
```
I'm on a mac so I'm going to use a docker to run a container and mount the
directory containing our example:
```console
$ docker run --privileged -ti -v $PWD/faas-js-example-oci:/root/faas-js-example-oci fedora /bin/bash
$ cd /root/faas-js-example-oci
$ dnf install -y runc
$ dnf install dnf-plugins-core
$ dnf copr enable ganto/umoci
$ dnf install umoci
```

We can now use `unoci` to unpack the image into a OCI bundle:
```console
$ umoci unpack --image faas-js-example-oci:latest faas-js-example-bundle
[root@2a3b333ff24b ~]# ls faas-js-example-bundle/
config.json  rootfs  sha256_be5c2a500a597f725e633753796f1d06d3388cee84f9b66ffd6ede3e61544077.mtree  umoci.json
```
`rootfs` will be the filesystem to be mounted and the configuration of the process
can be found in [config.json](https://github.com/opencontainers/runtime-spec/blob/master/config.md).

So we now have an idea of what a container is, a process, but what creates these
processes. This is the responsibility of a container runtime. 


### Container runtime
Docker contributed a runtime that they extracted named `runC`. There are others as well which I might
expand upon later but for now just know that this is not the only possibly runtime.

Something worth noting though is that these runtimes follow a specification that
describes what is to be run. These runtime operate on a [filesystem bundle](https://github.com/opencontainers/runtime-spec/blob/master/bundle.md)


We can run this bundle using [runC]():
```console
$ runc create --bundle faas-js-example-bundle faas-js-example-container
$ runc list
ID                          PID         STATUS      BUNDLE                         CREATED                        OWNER
faas-js-example-container   31          created     /root/faas-js-example-bundle   2019-12-09T12:55:40.8534462Z   root
```

runC does not deal with any image registries and only runs applications that are
packaged in the OCI format. So what ever executes runC would have to somehow
get the images into this format (bundle) and execute runC with that bundle.

So what calls runC?   
This is done by a component named `containerd` which is a container supervisor
(process monitor). It does not run the containers itself, that is done
by runC. Instead it deals with container lifecycle operations of containers run
by runC. Actually there is a runtime shim API allowing other runtimes to be used
instead of runC.


Containerd contains a Container Runtime Interface (CRI) API which is a gRPC API
. The API implementation uses the containerd Go client to call into containerd. 
Other clients that use the containerd Go client are Docker, Pouch, ctr.

```console
$ wget https://github.com/containerd/containerd/archive/v1.3.0.zip
$ unzip v1.3.0.zip
```

Building a docker image to play around with containerd and runc:
```console
$ docker build -t containerd-dev .
$ docker run -it --privileged \
    -v /var/lib/containerd \
    -v ${GOPATH}/src/github.com/opencontainers/runc:/go/src/github.com/opencontainers/runc \
    -v ${GOPATH}/src/github.com/containerd/containerd:/go/src/github.com/containerd/containerd \
    -e GOPATH=/go \
    -w /go/src/github.com/containerd/containerd containerd-dev sh

$ make && make install
$ cd /go/src/github.com/opencontainers/runc
$ make BUILDTAGS='seccomp apparmor' && make install

$ containerd --config config.toml
```
You can now attach to the same container and we can try out ctr and other commands:
```console
$ docker ps
$ docker exec -ti <CONTAINER_ID> sh
```
So lets try pulling an image:
```console
$ ctr image pull docker.io/library/alpine:latest
docker.io/library/alpine:latest:                                                  resolved       |++++++++++++++++++++++++++++++++++++++|
index-sha256:c19173c5ada610a5989151111163d28a67368362762534d8a8121ce95cf2bd5a:    done           |++++++++++++++++++++++++++++++++++++++|
manifest-sha256:e4355b66995c96b4b468159fc5c7e3540fcef961189ca13fee877798649f531a: done           |++++++++++++++++++++++++++++++++++++++|
layer-sha256:89d9c30c1d48bac627e5c6cb0d1ed1eec28e7dbdfbcc04712e4c79c0f83faf17:    done           |++++++++++++++++++++++++++++++++++++++|
config-sha256:965ea09ff2ebd2b9eeec88cd822ce156f6674c7e99be082c7efac3c62f3ff652:   done           |++++++++++++++++++++++++++++++++++++++|
elapsed: 2.5 s                                                                    total:  1.9 Mi (772.0 KiB/s)
unpacking linux/amd64 sha256:c19173c5ada610a5989151111163d28a67368362762534d8a8121ce95cf2bd5a...
done
```
The looks good. Next, lets see if we can run it:
```console
# ctr run docker.io/library/alpine:latest some_container_id echo "bajja"
bajja
```

So `containerd` is the daemon (long running background process) which exposes
a gRPC API over a local Unix socket (so there is not network traffic involved).
containerd supports the OCI Image Specification so any image that exists in upstream
repositories.
OCI Runtime Specification support allows any container runtime that support that
spec to be run, like `runC`, `rkt`.
Supports image pull and push.
A Task is a live running process on the system.

`ctr` is a command line tool for interacting with containerd. 

So, how could we run our above container using containerd?  
```console
$ docker exec -ti 78e22cb726b9 /bin/bash
$ cd /root/go/src/github.com/containerd/containerd/bin
$ ctr --debug images pull --user dbevenius:xxxx docker.io/dbevenius/faas-js-example:latest
```
The first thing that happens is containerd will fetch the the data from the remove, 
in this case docker and store this in the content store:
```console
$ ctr content ls
```
Fetch will update the metadata store and add a record that the image. The second
stage is the Unpack stage which will read the content and reads the layers from
the content store and unpack them into the snapshotter.

```console
$ ctr images ls
REF                                        TYPE                                                 DIGEST                                                                  SIZE     PLATFORMS  LABELS
docker.io/dbevenius/faas-js-example:latest application/vnd.docker.distribution.manifest.v2+json sha256:69cc8b6087f355b7e4b2344587ae665c61a067ee05876acc3a5b15ca2b15e763 28.9 MiB linux/amd64 -


$ ctr content ls | grep sha256:69cc8b6087f355b7e4b2344587ae665c61a067ee05876acc3a5b15ca2b15e763
DIGEST									SIZE	AGE		LABELS
sha256:69cc8b6087f355b7e4b2344587ae665c61a067ee05876acc3a5b15ca2b15e763	1.99kB	About an hour	containerd.io/gc.ref.content.2=sha256:95b3c812425e243848db3a3eb63e1e461f24a63fb2ec9aa61bcf5a553e280c07,containerd.io/gc.ref.content.4=sha256:28549a15ba3eb287d204a7c67fdb84e9d7992c7af1ca3809b6d8c9e37ebc9877,containerd.io/gc.ref.content.6=sha256:5a4ed7db773aa044d8c7d54860c6eff0f22aee8ee56d4badf4f890a3c82e6070,containerd.io/gc.ref.content.1=sha256:e7c96db7181be991f19a9fb6975cdbbd73c65f4a2681348e63a141a2192a5f10,containerd.io/gc.ref.content.7=sha256:aaf35efcb95f6c74dc6d2c489268bdc592ce101c990729280980da140647e63f,containerd.io/gc.ref.content.8=sha256:c79d77af46518dfd4e94d3eb3a989a43f06c08f481ab3a709bc5cd5570bb0fe2,containerd.io/gc.ref.content.3=sha256:778b81d0468fbe956db39aca7059653428a7a15031c9483b63cb33798fcdadfa,containerd.io/gc.ref.content.0=sha256:3e98616b38fe8a6943029ed434345adc3f01fd63dce3bec54600eb0c9e03bdff,containerd.io/distribution.source.docker.io=dbevenius/faas-js-example,containerd.io/gc.ref.content.5=sha256:0bcb2f6e53a714f0095f58973932760648f1138f240c99f1750be308befd943

$ ctr snapshots info faas-js-example-container
{
    "Kind": "Active",
    "Name": "faas-js-example-container",
    "Parent": "sha256:0e7d0af5a24eb910a700e2b293e4ae3b6a4b0ed5c277233ae7a62810cfe9c831",
    "Created": "2019-12-11T09:58:39.8149122Z",
    "Updated": "2019-12-11T09:58:39.8149122Z"
}
$ ctr snapshots tree
 sha256:f1b5933fe4b5f49bbe8258745cf396afe07e625bdab3168e364daf7c956b6b81
  \_ sha256:0a57385ee1dd96a86f16bfc33e7e8f3b03ba5054d663e4249e9798f15def762d
    \_ sha256:ebd0af597629452dee5e09da6b0bbecc93288d4910d49cef417097e1319e8e5f
      \_ sha256:fae0635457a678fa17ba41dc06cffc00c339c3c760515d8fd95f4c54d111ce4d
        \_ sha256:8e7ae562c333ef89a5ce0a5a49236ada5c7241e7788adbf5fe20fd3f6e2eb97d
          \_ sha256:323ec4a838fe67b66e8fa8e4fb649f569be22c9a7119bb59664c106c1af8e5b1
            \_ sha256:f4238a21a85c3d721b54f2304a671aa56cc593a436e2fe554f88369c527672f0
              \_ sha256:0e7d0af5a24eb910a700e2b293e4ae3b6a4b0ed5c277233ae7a62810cfe9c831
                \_ faas-js-example-container
```

So this is the information that will be available after a pull.

So we should now be able to run this image using ctr:
```console
$ ctr run docker.io/dbevenius/faas-js-example:latest faas-js-example-container
+ umask 000
+ cd /home/node/usr
+ '[' -f package.json ]
+ cd ../src
+ node .
{"level":30,"time":1576058320396,"pid":9,"hostname":"d51fc5895172","msg":"Server listening at http://0.0.0.0:8080","v":1}
FaaS framework initialized
```
Run read the image we want to run and create the OCI specification from it. It
will create a new read/write layer in the snapshotter. Then it will setup the
container which will have a new rootfs. When the runtime shim is asked to start
the process is will take the OCI specification and create a bundle directory:

```console
$ ls /run/containerd/io.containerd.runtime.v2.task/default/faas-js-example-container
address  config.json  init.pid	log  log.json  rootfs  runtime	work
```
We could use this directory to start a container just with `runc` if we wanted too:
```console
$ runc create -bundle /run/containerd/io.containerd.runtime.v2.task/default/faas-js-example-container/ faas-js-example-container2
$ runc list
ID                           PID         STATUS      BUNDLE                                                                            CREATED                        OWNER
faas-js-example-container2   5732        created     /run/containerd/io.containerd.runtime.v2.task/default/faas-js-example-container   2019-12-11T11:37:08.5385797Z   root
```

So we have launched a container using `ctr` which uses containerd go-client, and
the contains runtime used is runc.

We can attach another process and the inspect things:

List all the containers:
```console
$ ctr containers ls
CONTAINER                    IMAGE                                         RUNTIME
faas-js-example-container    docker.io/dbevenius/faas-js-example:latest    io.containerd.runc.v2
```
Get info about a specific container:
```console
$ ctr container info faas-js-example-container
{
    "ID": "faas-js-example-container",
    "Labels": {
        "io.containerd.image.config.stop-signal": "SIGTERM"
    },
    "Image": "docker.io/dbevenius/faas-js-example:latest",
    "Runtime": {
        "Name": "io.containerd.runc.v2",
        "Options": {
            "type_url": "containerd.runc.v1.Options"
        }
    },
    "SnapshotKey": "faas-js-example-container",
    "Snapshotter": "overlayfs",
    "CreatedAt": "2019-12-11T09:58:39.8637501Z",
    "UpdatedAt": "2019-12-11T09:58:39.8637501Z",
    "Extensions": null,
    "Spec": {
        "ociVersion": "1.0.1-dev",
        "process": {
            "user": {
                "uid": 1001,
                "gid": 0
            },
            "args": [
                "docker-entrypoint.sh",
                "/home/node/src/run.sh"
            ],
    ...
```
Notice the `SnapshotKey` which is `faas-js-example-container` and that it matches
the output from when we used the `ctr snapshots` command above.

Get information about running processes (tasks):
```console
# ctr tasks ls
TASK                         PID     STATUS
faas-js-example-container    5299    RUNNING
```
Lets take a look at that pid:
```console
# ps 5299
  PID TTY      STAT   TIME COMMAND
 5299 ?        Ss     0:00 /bin/sh /home/node/src/run.sh
```
So this is the actual container/process.

```console
$ ps aux | grep faas-js
root      5254  0.0  0.4 943588 26996 pts/1    Sl+  09:58   0:00 ctr run docker.io/dbevenius/faas-js-example:latest faas-js-example-container
root      5276  0.0  0.1 111996  6568 pts/0    Sl   09:58   0:00 /usr/local/bin/containerd-shim-runc-v2 -namespace default -id faas-js-example-container -address /run/containerd/containerd.sock
```
So process `5454` is the process we used to start the containers. Notice the
second process which is using `containerd-shim-runc-v2`

```console
# /usr/local/bin/containerd-shim-runc-v2 --help
Usage of /usr/local/bin/containerd-shim-runc-v2:
  -address string
    	grpc address back to main containerd
  -bundle string
    	path to the bundle if not workdir
  -debug
    	enable debug output in logs
  -id string
    	id of the task
  -namespace string
    	namespace that owns the shim
  -publish-binary string
    	path to publish binary (used for publishing events) (default "containerd")
  -socket string
    	abstract socket path to serve
```
This binary can be found in `cmd/containerd-shim-runc-v2/main.go`.
TODO: Take a closer look at how this is implemented.

So, we now have an idea of what is involved when running containerd and runc, 
and which process on the system we can inspect. We will now turn our attention
to kubernetes and kubelet to see how it uses containerd.


### Kubelet
In a kubernetes cluster, a worker node will have a kubelet daemon running which
processes pod specs and uses the information in the pod specs to start containers.

It originally did so by using docker as the container runtime. There are other
container runtime, for example rkt, and to be able to switch out the container
runtime an interface needed to be provided to enable this. This interface is 
called the Kubernetes Container Runtime Interface (CRI).

```
+------------+                 +--------------+      +------------+
| Kubelet    |                 |  CRI Shim    |      |  Container |<---> Container_0
|            |  CRI protobuf   |              |<---->|  Runtime   |<---> Container_1
| gRPC Client| --------------->| gRPC Server  |      |(containerd)|<---> Container_n
+------------+                 +--------------+      +------------+
```
The CRI Shim I think is a plugin in containerd enabling it to access lower level
services in containerd without having to go through the "normal" client API. This
might be useful to make a single API call that performs multiple containerd services
instead of having to go via the client API which might require multiple calls.


```console
$ docker run --privileged -ti fedora /bin/bash
$ dnf install -y kubernetes-node
$ dockerd
```
Next connect to the same container, remember just another process in the same
namespace etc:
```console
$ docker run -ti --privileged fedora /bin/bash
$ kubelet --fail-swap-on=false
```

#### Contents of an image
```console
$ docker build -t dbevenius/faas-js-example .
```
This filesystem is tarred (.tar) and metadata is added.

So, lets save an image to a tar:
```console
$ docker save dbevenius/faas-js-example -o faas-js-example.tar
```
If you extract this to location somewhere you can see all the files that
are included. 
```console
$ ls -l
total 197856
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 33f42e9c3b8312f301e51b6c2575dbf1943afe5bfde441a81959b67e17bd30fd
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 354bdf12df143f7bb58e23b66faebb6532e477bb85127dfecf206edf718f6afa
-rw-r--r--  1 danielbevenius  staff      7184 Nov 25 09:37 3e98616b38fe8a6943029ed434345adc3f01fd63dce3bec54600eb0c9e03bdff.json
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 4ce67bc3be70a3ca0cebb5c0c8cfd4a939788fd413ef7b33169fdde4ddae10c9
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 835da67a1a2d95f623ad4caa96d78e7ecbc7a8371855fc53ce8b58a380e35bb1
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 86b808b018888bf2253eae9e25231b02bce7264801dba3a72865af2a9b4f6ba9
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 91859611b06cec642fce8f8da29eb8e18433e8e895787772d509ec39aadd41f9
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 b7e513f1782880dddf7b47963f82673b3dbd5c2eeb337d0c96e1ab6d9f3b76bd
drwxr-xr-x  5 danielbevenius  staff       160 Nov 25 09:37 f3d9c7465c1b1752e5cdbe4642d98b895476998d41e21bb2bfb129620ab2aff9
-rw-r--r--  1 danielbevenius  staff       794 Jan  1  1970 manifest.json
-rw-r--r--  1 danielbevenius  staff       183 Jan  1  1970 repositories
```

manifest.json:
```console
[
  {"Config":"3e98616b38fe8a6943029ed434345adc3f01fd63dce3bec54600eb0c9e03bdff.json",
   "RepoTags":["dbevenius/faas-js-example:0.0.3","dbevenius/faas-js-example:latest"],
    "Layers":["b7e513f1782880dddf7b47963f82673b3dbd5c2eeb337d0c96e1ab6d9f3b76bd/layer.tar",
              "86b808b018888bf2253eae9e25231b02bce7264801dba3a72865af2a9b4f6ba9/layer.tar",
              "354bdf12df143f7bb58e23b66faebb6532e477bb85127dfecf206edf718f6afa/layer.tar",
              "4ce67bc3be70a3ca0cebb5c0c8cfd4a939788fd413ef7b33169fdde4ddae10c9/layer.tar",
              "91859611b06cec642fce8f8da29eb8e18433e8e895787772d509ec39aadd41f9/layer.tar",
              "835da67a1a2d95f623ad4caa96d78e7ecbc7a8371855fc53ce8b58a380e35bb1/layer.tar",
              "f3d9c7465c1b1752e5cdbe4642d98b895476998d41e21bb2bfb129620ab2aff9/layer.tar",
              "33f42e9c3b8312f301e51b6c2575dbf1943afe5bfde441a81959b67e17bd30fd/layer.tar"]}]
```
repositories:
```console
{
  "dbevenius/faas-js-example": {
     "0.0.3":"33f42e9c3b8312f301e51b6c2575dbf1943afe5bfde441a81959b67e17bd30fd",
     "latest":"33f42e9c3b8312f301e51b6c2575dbf1943afe5bfde441a81959b67e17bd30fd"
  }
}
```

3e98616b38fe8a6943029ed434345adc3f01fd63dce3bec54600eb0c9e03bdff.json:
This file contains the configuration of the container.

When we build a Docker image we specify a base image and that is usually a
specific operating system. This is not a full OS but instead all the libraries
and utilities expected to be found by the application. They kernel used is the
host. 

#### Pods
Is a group of one or more containers with shared storage and network. Pods are
the unit of scaling.

A pod consists of a Linux namespace which is shared with all the containers in 
the pod, which gives them access to each other. So a container is used for
isolation you can join them using namespaces which how a pod is created. This
is how a pod can share the one IP address as they are in the same networking
namespace. And remember that a container is just a process, so these are multiple
processes that can share some resources with each other.

#### Kubernetes Custom Resources
Are extentions of the Kubernetes API. A resource is simply an endpoint in the
kubernetes API that stores a collection of API objects (think pods or deployments
and things like that). You can add your own resources just like them using 
custom resources. After a custom resources is installed kubectl can be used to
with it just like any other object.

So the customer resource just allows for storing and retrieving structured data,
and to have functionality you have custom controllers.

#### Controllers
Each controller is responsible for a particular resource.

Controller components:

##### Informer/SharedInformer
A resource can be watched which is a verb in the exposed REST API. When this is used
there will be a long running connection, a http/2 stream, of event changes to
the resource (create, update, delete, etc). 

Watches the current state of resource instances and sends events to the
Workqueue. The informer gets the information about an object it sends a request
to the API server. Instead of each informer caching the objects it is interested
in multiple controllers might be interested in the same resource object. Instead
of them each caching the data/state they can share the cache among themselves,
this is what a SharedInformer does.

The informers also contain error handling for the long running connection breaks
, it will take care of reconnecting. 

Resource Event Handler handles the notifications when changes occur.
```rust
type ResourceEventHandlerFuncs struct {
	AddFunc    func(obj interface{})
	UpdateFunc func(oldObj, newObj interface{})
	DeleteFunc func(obj interface{})
}
```

##### Workqueue
Items in this queue are taken by workers to perform work.

#### Custom Resource Def/Controller example
[rust-controller](./rust-controller) is an example of a custom resource controller
written in Rust. The goal is to understand how these work with the end goal being
able to understand how other controllers are written and how they are installed
and work. 

I'm using CodeReady Container(crc) so I'll be using some none kubernetes commands:
```
$ oc login -u kubeadmin -p e4FEb-9dxdF-9N2wH-Dj7B8 https://api.crc.testing:6443
$ oc new-project my-controller
$ kubectl create  -f k8s-controller/docs/crd.yaml
customresourcedefinition.apiextensions.k8s.io/members.example.nodeshift.com created
```
We can try to access `somthings` using:
```console
$ kubectl get member -o yaml
```
But there will not be anything in there get. We have to create something using
```console
$ kubectl apply -f k8s-controller/docs/member.yaml
member.example.nodeshift.com/dan created
```
Now if we again try to list the resources we will see an entry in the `items` list.
```console
$ kubectl get members -o yaml -v=7
```
The extra `-v=7` flag gives verbose output and might be useful to know about.

And we can get all Something's using:
```console
$ kubectl get Member
$ kubectl describe Member
$ kubectl describe Member/dan
```

You can see all the available short names using `api-resources`
```console
$ kubectl api-resources
```

```console
$ kubectl config current-context
default/api-crc-testing:6443/kube:admin
```

### go-controller
This is a controller written in go. The motivation for having two is that most
controllers I've seen are written in go and having an understanding of the code
and directory structure of one will help understand others.

First to get all the dependencies onto our system we are going to use 
sample-controller from the kubernetes project:
```console
$ go get k8s.io/sample-controller
```
We should now be able to build our `go-controller`:
```
$ unset CC CXX
$ cd go-controller 
$ go mod vendor
$ go build -o go-controller .
```

```console
$ ./go-controller -kubeconfig=$HOME/.kube/config
```


Building/Running:
```
$ cargo run
```
Deleting a resource should trigger our controller:
```console
$ kubectl delete -f docs/member.yaml
```

Keep this in mind when we are looking at Knative and Istio that this is mainly
how one extends kubernetes using customer resources definitions with controllers.

### Installation
Knative runs on kubernetes, and Knative depends on Istio so we need to install
these. Knative depends on Istio for setting up the internal network routing and the
ingress (data originating from outside the local network).

I'm using mac so I'll use home brew to install minikube:
```console
$ brew install minikube
```
Next, we start minikube:
```console
$ minikube start --memory=8192 --cpus=6 \
  --kubernetes-version=v1.15.0 \
  --vm-driver=hyperkit \
  --disk-size=30g \
  --extra-config=apiserver.enable-admission-plugins="LimitRanger,NamespaceExists,NamespaceLifecycle,ResourceQuota,ServiceAccount,DefaultStorageClass,MutatingAdmissionWebhook"
```
We are using 1.15.0 as this is the version the Istio is compatible with.
Also, we need to have a version of `kubectl` that matches this version:
```console
$ curl -LO https://storage.googleapis.com/kubernetes-release/release/v1.15.0/bin/linux/amd64/kubectl
$ chmod +x kubectl 
$ sudo mv ./kubectl /usr/local/bin/kubectl 
```

So, we should now have a kubernetes cluster up and running. 
```console
$ minikube status
host: Running
kubelet: Running
apiserver: Running
kubeconfig: Configured

```
Show the status of the Control Plane components:
```console
$ kubectl get componentstatuses
NAME                 STATUS    MESSAGE             ERROR
controller-manager   Healthy   ok                  
scheduler            Healthy   ok                  
etcd-0               Healthy   {"health":"true"}   
```

We now want to add Istio to it.
```console
$ curl -L https://istio.io/downloadIstio | sh -
$ kubectl apply -f istio-1.1.7/install/kubernetes/istio-demo.yaml
```

Wait for the pods to be created:
```console
$ kubectl get pods -n istio-system --watch
```

Now we are ready to install Knative:
```console
$ kubectl apply --selector knative.dev/crd-install=true \
   --filename https://github.com/knative/serving/releases/download/v0.10.0/serving.yaml \
   --filename https://github.com/knative/eventing/releases/download/v0.10.0/release.yaml \
   --filename https://github.com/knative/serving/releases/download/v0.10.0/monitoring.yaml
```
And now once more:
```console
$ kubectl apply --filename https://github.com/knative/serving/releases/download/v0.10.0/serving.yaml \
   --filename https://github.com/knative/eventing/releases/download/v0.10.0/release.yaml \
   --filename https://github.com/knative/serving/releases/download/v0.10.0/monitoring.yaml
```

To verify that Knative has been installed we can check the pods:
```console
$ kubectl get pods --namespace knative-serving --namespace=knative-eventing
```

So, after this we should be good to go and deploying a Knative service should
be possible:
```console
$ kubectl apply -f service.yaml
$ kubectl get pods --watch
```

```console
$ kubectl describe svc js-example
```

Get the port:
```console
$ kubectl get svc istio-ingressgateway --namespace istio-system --output 'jsonpath={.spec.ports[?(@.port==80)].nodePort}'
31380
```
And the ip:
```console
$ minikube ip
192.168.64.21
```

We can now use this information to invoke our service:
```console
$ curl -v -H 'Host: js-example.default.example.com' 192.168.64.21:31380/
```
You'll notice that it might take a while for the first call if the service
has been scaled down to zero. You'll can check this by first seeing if there
are any pods before you run the curl command and then afterwards.

If you have stopped and restarted the cluster (perhaps because the noise of your
computer fan was driving you crazy) you might get the following error message:
```console
UNAVAILABLE:no healthy upstream* Closing connection 0
```
The service will eventually become avilable and I think my machine (from 2013)
is just exceptionally slow for this type of load). 

So we have a service and we want events to be delivered to it. For this we
need something that sends events. This is called an event source in knative. 


```console
$ kubectl apply -f source.yaml
```

Such an event source can send events directly to a service but that means that the
source will have to take care of things like retries and handle situations when
the service is not available. Instead the event source can use a channel which it
can send the events to. 
```console
$ kubectl apply -f channel.yaml
```

Something can subscribe to this channel enabling the event
to get delivered to the service, these things are called subscriptions.

```console
$ kubectl apply -f subscription.yaml
```

So we have our service deployed, we have a source for generating events which
sends events to a channel, and we have a subscription that connects the channel
to our service. Lets see if this works with our js-example.

Sometimes when reading examples online you might copy one and it fails to deploy
saying that the resoures does not exist. For example:
```console
error: unable to recognize "channel.yaml": no matches for kind "Channel" in version "eventing.knative.dev/v1alpha1"
```
If this happens and you have installed a channel resource you an use the following
command to find the correct `apiVersion` to use:
```
$ kubectl api-resources | grep channel
channels                          ch              messaging.knative.dev              true         Channel
inmemorychannels                  imc             messaging.knative.dev              true         InMemoryChannel

Next we will create a source for events:

```console
$ kubectl describe sources
```

Knative focuses on three key categories:
```
* building your application
* serving traffic to it 
* enabling applications to easily consume and produce events.
```

### Serving
Automatically scale based on load, including scaling to zero when there is no load.
You deploy a prebuilt image to the underlying kubernetes cluster.

Serving contains a number of components/object which are described below:

#### Configuration
This will contain a name reference to the container image to deploy. This
ref is called a Revision.
Example configuration (configuration.yaml):
```yaml
apiVersion: serving.knative.dev/v1alpha1
kind: Configuration
metadata:
  name: js-example
  namespace: js-event-example
spec:
  revisionTemplate:
    spec:
      container:
        image: docker.io/dbevenius/faas-js-example
```
This can be applied to the cluster using:
```console
$ kubectl apply -f configuration.yaml
configuration.serving.knative.dev/js-example created
```

```console
$ kubectl get configurations js-example -oyaml
```

```
$ kubectl get ksvc js-example  --output=custom-columns=NAME:.metadata.name,URL:.status.url
NAME               URL
js-example   http://js-example.default.example.com
```

#### Revision
Immutable snapshots of code and configuration. Refs a specific container image
to run. Knative creates Revisions for us when we modify the Configuration.
Since Revisions are immutable and multiple versions can be running at once,
it’s possible to bring up a new Revision while serving traffic to the old version.
Then, once you are ready to direct traffic to the new Revision, update the Route
to instantly switch over. This is sometimes referred to as a blue-green
deployment, with blue and green representing the different versions.
```console
$ kubectl get revisions 
```

#### Route
Routes to a specific revision.

#### Service
This is our functions code.

The serving names space is `knative-serving`. The Serving system has four
primary components. 
```
1) Controller
Is responsible for updating the state of the cluster. It will create kubernetes
and istio resources for the knative-serving resource being created.

2) Webhook
Handles validation of the objects and actions performed

3) Activator
Brings back scaled-to-zero pods and forwards requets.

4) Autoscaler
Scales pods as requests come in.
```
We can see these pods by running:
```console
$ kubectl -n knative-serving get pods
```

So, lets take a look at the controller. The configuration files for are located
in [controller.yaml](https://github.com/knative/serving/blob/master/config/controller.yaml)
```console
$ kubectl describe deployment/controller -n knative-serving
```
I'm currently using OpenShift so the details compared to the controller.yaml will
probably differ but the interesting part for me is that these are "just" object
that are deployed into the kubernetes cluster.

So what happens when we run the following command?
```console
$ kubectl apply -f service.yaml
```
This will make a request to the API Server which will take the actions appropriate
for the description of the state specified in service.yaml. For this to
work there must have been something registered that can handle the apiversion:
```
apiVersion: serving.knative.dev/v1alpha1
```
I'm assuming this is done as part of installing knative 

### Eventing
Makes it easy to produce and consume events. Abstracts away from event sources
and allows operators to run their messaging layer of choice.

Knative is installed as a set of Custom Resource Definitions (CRDs) for
Kubernetes.

#### Sources
The source of the events.
Examples:
* GCP PubSub
* Kubernetes Events
* Github
* Container Sources



#### Channels
While you can send events straight to a Service, this means it’s up to you to
handle retry logic and queuing. And what happens when an event is sent to your
Service and it happens to be down? What if you want to send the same events to
multiple Services? To answer all of these questions, Knative introduces the
concept of Channels.
Channels handle buffering and persistence, helping ensure that events are
delivered to their intended Services, even if that service is down. Additionally,
Channels are an abstraction between your code and the underlying messaging
solution. This means we could swap this between something like Kafka and RabbitMQ.

Each channel is a custom resource.


#### Subscriptions
Subscriptions are the glue between Channels and Services, instructing Knative
how our events should be piped through the entire system.


### Istio
Istio is a service mesh that provides many useful features on top of Kubernetes
including traffic management, network policy enforcement, and observability. We
don’t consider Istio to be a component of Knative, but instead one of its
dependencies, just as Kubernetes is. Knative ultimately runs on a Kubernetes
cluster with Istio.

### Service mesh
A service mesh is a way to control how different parts of an application share
data with one another. So you have your app that communicates with various other
sytstems, like backend database applications or other systems. They are all
moving parts and their availability might change over time. To avoid one system
getting swamped with requests and overloaded a service mesh is used which routes
requests from one service to the next. This indirection allows for optimizations
and re-routing where needed.

Another reasons for having a service mesh like this is that a microservice
architecture might be implemented in various different languages. These languages
have different ways of doing things like providing stats, tracing, logging,
retry, circuit breaking, rate limiting, authentication and authorization.
This can make it difficult to debug latency and failures.

In a service mesh, requests are routed between microservices through proxies in
their own infrastructure layer. For this reason, individual proxies that make
up a service mesh are sometimes called “sidecars,” since they run alongside each
service, rather than within them. These sidecars are just containers in the pod.
Taken together, these “sidecar” proxies—decoupled from each service—form a mesh
network.

So each service has a proxy attached to it which is called a sidecar. These 
side cars route network request to other side-cars, which are the services
that the current service uses. The network of these side cars are the service
mesh.

These sidcars also allow for collecting metric about communication so that other
services can be added to monitor or take actions based on changes to the network.
The sidecar will do things like service discovery, load balancing, rate limiting,
circuit breaking, retry, etc.
So if serviceA want to call serviceB, serviceA will talk to its local sidecar
proxy which will take care of calling serviceB, where ever that serviceB might
be at the current time. So there services them are decoupled from each other
and also don't have to be concerned with networking, they just communicate with
the local sidecar proxy.

Note that we have only been talking about communication between services and
not communication with the outside world (outside of the service network/mesh).
To expose a service to the outside world and allow it to be access through the
service mesh, so that it can take advantage of all the features like of the
service mesh instead of calling the service directly, we have to enble ingress
traffic.

So we have dynamic request routing in the proxies. 
To manage the routing and other features of the service mesh a control plane
is used for centralized management.
In Istio this is called a control plan which has three components:
```
1) Pilot
2) Mixer
2) Istio-Auth
```

### Sidecar
Is a utility container that supports the main container in a pod. Remember that
a pod is a collection of one or more containers.

All of these instances form a mesh and share routing information with each other.

So to use Knative we need istio for the service mesh (communication between
services), we also need to be able to access the target service externally which
we use some ingress service for. 

So I need to install istio (or other service mesh) and an ingress to the
kubernetes cluster and then Knative to be able to use Knative.

### Istio
Is a service mesh implementation and also a platform, including APIs that let
it integrate into any logging platform, or telemetry or policy system

You add Istio support to services by deploying a special sidecar proxy throughout
your environment that intercepts all network communication between microservices,
then configure and manage Istio using its control plane functionality

Istio’s traffic management model relies on the Envoy proxies that are deployed
along with your services.  All traffic that your mesh services send and receive
(data plane traffic) is proxied through Envoy, making it easy to direct and
control traffic around your mesh without making any changes to your services.

### Envoy
The goal on Envoy is to make the network transparent to applications. When issues
occur they should be easy to figure out where the problem is.

Envoy is an out of process architecture which is great when you have services
written in multiple languages. If you opt for a library approach you have to
have implementations in all the languages that you use (hysterix is an example).
Envoy is a layer3/layer4 filter architecture (so network layer (IP), and transport
layer (TCP/UDP). There is also a layer 7 (application layer) that can operate/filter
http headers.

Service discovery and active (ping the service)/passive (monitor the trafic) health
checking.
Has various load-balancing algorithms.
Provides observability via stats, logging, and tracing.
Authentication and authorization

Envoy is used as both an Edge proxy and a service proxy.
1) Edge proxy
This gives a single point of ingress (external traffic; not internal to the
service mesh).
2) Service proxy
This is a separate process that keeps an eye on the services. 

### CloudEvent spec 1.0
Mandatory:
```
id		string identifier
source          url that identifies the context in which the event happend
                type of event, etc. The source+id must be unique for each event.

specversion
type
```
Optional:
```
datacontenttype
dataschema
data
subject
time
```
Example:
```json
{
    "specversion" : "1.0",
    "type" : "com.github.pull.create",
    "source" : "https://github.com/cloudevents/spec/pull",
    "subject" : "123",
    "id" : "A234-1234-1234",
    "time" : "2018-04-05T17:31:00Z",
    "comexampleextension1" : "value",
    "comexampleothervalue" : 5,
    "datacontenttype" : "text/xml",
    "data" : "<much wow=\"xml\"/>"
}
```


There is a http-protocol-binding specification:
https://github.com/cloudevents/spec/blob/v1.0/http-protocol-binding.md

This spec defines three content modes for transferring events:
```
1) binary
2) structured
3) batched
```

In the binary content mode, the value of the event data is placed into the HTTP
request/response body as-is, with the datacontenttype attribute value declaring
its media type in the HTTP Content-Type header; all other event attributes are
mapped to HTTP headers.

In the structured content mode, event metadata attributes and event data are
placed into the HTTP request or response body using an event format.

Event formats:
These formats are used with structured content mode.

This format can be one of different specs, for example there is one spec for
a json format (https://github.com/cloudevents/spec/blob/v1.0/json-format.md).

```
Content-Type: application/cloudevents+json; charset=UTF-8
```

`datacontenttype` is expected to contain a media-type expression, for example
`application/json;charset=utf-8`.  

`data` is encoded using the above media-type.

The content mode is chosen by the sender of the event.
The receiver of the event can distinguish between the three modes by inspecting
the Content-Type header value. If the value of this header is `application/cloudevents`
it is a structured mode, if `application/cloudevents-batch` then it is batched,
otherwise it the mode is binary.

### HTTP Header names
These headers all have a `ce-` prefix.


### Helm
Is a package manager for Kubernetes (think npm).
Helm calls its packaging format charts which is a collection of files related
to a set of Kubernetes resources.
A chart must follow a naming and directory convention. The name of the dirctory
must be the name of the chart:
```
chartname/
         Chart.yaml
         values.yaml
         charts
         crds (Custom Resourcde Definitions)
         templates
```


### kubectl
Remove all the objects defined in a yaml file:
```console
kubectl delete -f service.yaml
```
Update the object defined in a yaml file:
```console
kubectl replace -f service.yaml
```


The Operator pattern combines custom resources and custom controllers.

### Kubernetes
Kubernetes is a portable, extensible, open-source platform for managing
containerized workloads and services, that facilitates both declarative
configuration and automation. It gives you service discovery and load balancing,
storage orchestration (mounting storage systems), automated rollouts/rollbacks,
self healing, etc.
There are various components to kubernetes:

#### Master node
Acts as a controller and is where decisions are made about scheduling, detecting
and responding to cluster events.
The master consists of the following components:

##### API Server
Exposes REST API that users/tools can interact with.

##### Cluster data store
This is `etcd` which is a key-value data store and is the persistent store that
kubernetes uses.

##### Controller Manager
Runs all the controllers that handle all the routine tasks in the cluster. Examples
are Node Controller, Replication Controller, Endpoint Controller, Service Account,
and Token Controllers.

##### Scheduler
Watches for new pods and assigns them to nodes.

##### Dashboard (options)
Web UI.


#### Resource
A resource is an object that is stored in etcd and can be accessed through the
api server. By itself this is what it does, contains information about the resource
. A controller is what performs actions as far as I understand it.


#### Worker nodes
These run the containers and provide the runtime. A worker node is comprised of
a kublet. It watches the API Server for pods that have been assigned to it.
Inside each pod there are containers. Kublet runs these via Docker by pulling
images, stopping, starting, etc.
Another part of a worker node is the kube-proxy which maintains networking rules
on the host and performing connection forwarding. It also takes care of load
balancing.


#### Kubernetes nodes
Each node has a `kubelet` process and a `kube-proxy` process.

##### Kubelet
Makes sure that the containers are running in the pod. The information it uses
is the PodSpecs.

##### Kube-proxy
Just like kubelet is responsible for starting containers, kubeproxy is responsible
for making services work. It watches for service events and creates, updates,
or deletes kube-proxies on the worker node.
Maintains networking rules on nodes. It uses the OS packet filtering layer if
available.

##### Container runtime
Is the software responsible for running containers (docker, containerd, cri-o,
rktlet).


#### Addons

##### Cluster DNS addon
Is a DNS server which serves DNS records for kubernetes services.
Containers started by Kubernetes automatically include this DNS server in their DNS searches

### API Groups
Kubernetes uses a versioned API which is categoried into API groups. For example,
you might find:
```
apiVersion: serving.knative.dev/v1alpha1
```
in a yaml file which is specifying the API group, `serving.knative.dev` and the
version.

#### Pods
Is a group of one or more containers with shared storage and network. Pods are
the unit of scaling.
A pod consists of a Linux namespace which is shared with all the containers in 
the pod, which gives them access to each other. So a container is used for
isolation, you can join them using namespaces which how a pod is created. This
is how a pod can share the one IP address as they are in the same networking
namespace.

#### ReplicaSet
The goal of a replicaset is to maintain a stable set of replica Pods.
When a ReplicaSet needs to create new Pods, it uses its Pod template.
Deployment is a higher-level concept that manages ReplicaSets and provides
declarative updates to Pods along with a lot of other useful features. It is
recommended to use Deployments instead of ReplicaSets directly.

#### ReplicationController
ReplicationController makes sure that a pod or a homogeneous set of pods is
always up and available. If there are too many pods this controller will delete
them, and if there are not enough it will create more. Pods maintined by this
controller are automatically replace if deleted, which is not the case for manually
create pods.


### Container networking
Docker networking uses the kernel's networking stack as low level primitives to
create higher level network drivers.

We have seen how a linux bridge can be used to connect containers on the same
host. But what if we have containers on different host that need to communicate?  
There are multiple solutions to this and one is using VXLAN, or a Macvlan and
perhaps others. As these are new to me I'm going to go through what they are
to help me understand networking in kubernetes better.

#### Virtual Local Area Network (VLAN)
To separate two networks they had to be physically separated. For example, you
might have a guest network which should not be allowed to connect to the internal
network. These two should not be able to communicate with each other so there
was simply no connection between the hosts on one network to hosts on the other.
The hosts would be connected to separate switches.

VLANs provide logical separation/segmentation, so we can have all hosts connected
to one switch but they can still be separated into separate logical networks.
This also allows host to be located a different locations as they don't have to
be connected to the physical switch (which was not possible with pre-vlan). With
vlan it does not matter where the hosts are, different floors/building/locations.

#### Virtual Extended Local Area Network (VXLAN)
VLANs are limited to 4094 VLANs but VXLANS allow for more than 4094 which might
be required in a cloud environment. Is supported by the linux kernel and is a
network tunnel. VXLAN tunnels layer 2 frames inside of UDP datagrams. This means
that containers that are part of the same virtual local area network are on the
same L2 network, but infact they are separated.

#### Macvlan
Macvlan provides MAC hardware addresses to each container allowing them to become
part of the traditional network and use IPAM or VLAN trunking.

#### Overlay network
The physical network is called the underlay and an overly abstracts this to
create a virtual network. Much like we did in the example of using a bridge in
the networking example in this case a virtual tunnel endpoint (VTEP) is added
to the bridge. This will then encapsulate the packet in a udp datagram with a
few additional headers.
VTEPs get their own MAC and IP addresses and show up as network interfaces 


### Kubernetes Networking
So, we understand that containers in the same namespace is what a pod is. And
they will share the same network namespace hence have the same ip address, and
share the same iptables, and ip routing rules. In the namespaces section above
we also saw how multiple namespaces and communicate with each other.

So, a pod will have an ip address (all the processes in the same namespace) and
the worker node that the pod is running on will also have a ip:
```
+--------------------------+
|     worker node0         |
|  +------------+          |
|  |     pod    |          |
|  | 172.16.0.2 |          |
|  +------------+          |
|                          |
|      ip: 10.0.1.3        |
|pod cidr: 10.255.0.0/24   |
+--------------------------+
```

```
+-------------------------------------+
|     service                         |
|selector:                            |
|port:80:7777                         |
|port:8080:7777                       |
|type:ClusterIP|NodePort|LoadBalancer |
+-------------------------------------+
```
The `ClusterIP` is assigned by a controller manager for this service. This will
be unique across the whole cluster. This can also be a dns name.
So you can have applications point to the ClusterIP and even if the underlying
target pods are moved/scaled they will still continue to work. There is really
nothing behind the ClusterIP, like there is no container or anything like that.
Instead the cluster ip is a target for iptables. So when a packet destined for
the cluster ip address it will get routed by iptables to the actual pods that
implement that service. 

Kubeproxy will watch for services and endpoints and update iptables on that worker
node. So if an endpoint is removed iptables can be updated to remove that entry.

The `NodePort` type deals with getting traffic from outside of the cluster. 
```
+-------------------------------------+
|     service                         |
|selector:                            |
|port:32599:80:7777                   |
|type:NodePort                        |
+-------------------------------------+
```
The port `32599` will be an entry in iptables for each node. So we can now use
the `nodeip:32599` to get to the service. 

The `LoadBalancer` type is cloud specific and allows for a nicer was to access
services from outside the cluster and not having to use the `nodeip:port`. The
load balancer will still point to the NodePort so it builds on top of it.

### IPTables
Is a firewall tool that interfaces with the linux kernel `netfilter` subsystem.
Kube-proxy attaches rules to the PRE_ROUTING for services.

```console
$ iptables -t nat -A PREROUTING -match conntrack -ctstate NEW -j KUBE_SERVICE
```
Above, we are adding a rule to the `nat` table by appending to the `PREROUTING`
chain. The `match` specifies a iptables-extension which is specified as `conntrack`
which allows access to the connection tracking state for this packet/connection.
By specifying this extension we also can specify `ctstate` as `NEW`. Finally,
the target is specified as `KUBE_SERVICE.

```console
$ iptables -A KUBE_SERVICES ! -s src_cidr -d dst_cidr -p tcp -m tcp --dport 80 -j KUBE_MARQ
```
This appends a rule to the KUBE_SERVICES chain. 
TODO: add this from a real example.

When a cluster grows to many services using iptables can be a cause of performace
issues as it will be applied sequentially. The packet will be checked against the
rule one by one until it is found, O(n). For this reason IP Virtual Server (IPVS)
which is also a Linux kernel feature can be used. I think that this was also a
reason for looking into other ways to avoid this overhead, and one such way is
to use [eBPF](https://lwn.net/Articles/740157/). 

#### Container Network Interface (CNI)
The following is from the [cni-plugin](https://github.com/containernetworking/cni/blob/master/SPEC.md#cni-plugin) 
section:
```
A CNI plugin is responsible for inserting a network interface into the container
network namespace (e.g. one end of a veth pair) and making any necessary changes
on the host (e.g. attaching the other end of the veth into a bridge). It should
then assign the IP to the interface and setup the routes consistent with the IP
Address Management section by invoking appropriate IPAM plugin.
```
This should hopefully sound familiar and is very similar to what we did in the
network namespace section. 
The kubelet specifies the CNI to be used as a command line option.


### tun/tap
Are software only interfaces

```console
$ docker run --privileged -ti -v$PWD:/root/learning-knative -w/root/learning-knative gcc /bin/bash
$ mkdir /dev/net
$ mknod /dev/net/tun c 10 200
$ ip tuntap add mode tun dev tun0
$ ip addr add 10.0.0.0/24 dev tun0
$ ip link set dev tun0 up
$ ip route get 10.0.0.2

$ gcc -o tun tun.c
$ ./tun
Device tun0 opened
```


#### Building Knative Eventing
I've added a few environment variables to .bashrc and I also need to login
to docker:
```console
$ source ~/.bashrc
$ docker login
```
```console
$ mkdir -p ${GOPATH}/src/knative.dev
```

Running the unit tests:
```console
$ go test -v ./pkg/...
# runtime/cgo
ccache: invalid option -- E
Usage:
    ccache [options]
    ccache compiler [compiler options]
    compiler [compiler options]          (via symbolic link)
```
I had to unset CC and CXX for this to work:
```console
$ unset CC
$ unset CXX
```

### go-client
Is a web service client library written in go (k8s.io/client-go).


### Kubernetes on AWS
You can setup an Amazon Elastic Computing (EC2) node on thier freetier which only
has one CPU. It is possible to run minikube on that with a flag to avoid a CPU
check as shown below:
```console
$ ssh -i ~/.ssh/aws_key.pem ec2-user@ec2-18-225-37-245.us-east-2.compute.amazonaws.com
$ sudo yum update
$ sudo yum install -y docker
$ curl -Lo minikube https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
$ sudo -i
$ /usr/local/bin/minikube start --vm-driver=none --extra-config=kubeadm.ignore-preflight-errors=NumCPU --force --cpus 1
$ sudo mv /root/.kube /root/.minikube $HOME
$ sudo chown -R $USER $HOME/.kube $HOME/.minikube
```
After this it should be possible to list the resources available on the cluster:
```console
$ kubectl api-resources
```
Now, we want to be able to interact with this cluster from our local machine. To
enable this we need to add a configuration for this cluster to ~/.kube/config:
TODO: figure out how to set this up.
In the meantime I'm just checking out this repository on the same ec2 instance
and using a github personal access token to be able to work and commit.


### Operators
In OpenShift Operators are the preferred method of packaging, deploying, and
managing services on the control plane. 

### API Gateway
An API Gateway is focused on offering a single entry point for external clients
whereas a service mesh is for service to service communication. But there are
a lot of features that both have in common. But there would be more overhead having
an API gateway between all internal services (like latency for example).
Ambassidor is an example of an API gateway.
But an API gateway can be used at the entry point to a service mesh.


### gRPC
A service is created by defining the functions it exposes and this is what the
server application implements. The server side runs a gRPC server to handle calls
to the service by decoding the incoming request, executing the method, and encoding
the response. The service is defined using an interface definition language (IDL). 
gRPC can use protocol buffers (see Protocol Buffers for more details) as its IDL
and as its message format.

The clients then generate a stub and can call functions on them locally. The nice
thing is that clients can be generated in different languages, so the service
could be written in one and then multiple clients stubs generated for languages
that support gPRC.

Just to give some context where gRPC is coming from is that it is being used
in stead of Restful APIs in places. Restful APIs don't have a formal machine readable
API contract. The clients need to be written. Streaming is difficult. The information
sent over the wire is not efficient for networks. And many restful endpoints are
not actually resources (get created with put/post, retreived using get, etc).

gRPC is a protocol built on top of HTTP/2.
There are three implementations:
* C core - which is used by Ruby, Python, Node.js, PHP, C#, Objective-C, C++
* Java (Netty + BoringSSL)
* Go

gRPC was originally developed as Google and the the internal name was `stubby`.

### Protocol Buffers (protobuf)
Is a mechanism for serializing structured data. You create file that defines the
structure with a `.proto` extension:
```console
message Something {
  string name;
  int32 age;
}
```
With this file created we can use the compiler `protoc` to generate data access 
classes in the language you choose.

A gRPC service is be created and it will use `message` types as the types of
parameters and return values. The services themselves are specified using `rcp`:
```
syntax = "proto3";

package lkn;

service Something {
  rpc doit (InputMsg) returns (OutputMsg);
}

message InputMsg{
  string input = 1;
}

message OutputMsg{
  string output = 1;
}
```
The [gprc](./gprc) contains a node.js example of gRPC server and client.

### Minikube on RHEL 8 issues
```console
💣  Unable to start VM. Please investigate and run 'minikube delete' if possible: create: Error creating machine: Error in driver during machine creation: ensuring active networks: starting network minikube-net: virError(Code=89, Domain=47, Message='The name org.fedoraproject.FirewallD1 was not provided by any .service files')

😿  minikube is exiting due to an error. If the above message is not useful, open an issue:
👉  https://github.com/kubernetes/minikube/issues/new/choose

```
I was able to work around this by running:
```console
$  sudo systemctl restart libvirtd
```
This was still not enough to get minikube to start though, I needed to delete
minikube and start again:
```
$ minikube delete
$ minikube start
```
