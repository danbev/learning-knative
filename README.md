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

Also it is worth mentioning that a running container is process (think unix process)
which has a separate control group (cgroup), and namespace (mnt, IPC, net, usr, pid,
and uts (Unix Time Share system)).

##### cgroups
cgroups allows the Linux OS to manage and monitor resources allocated to a process
and also set limits for things like CPU, memory, network. This is so that one
process is not allowed to hog all the resources and affect others. 

##### namespaces
Are used to isolate process from each other. Each container will have its own
namespace but it is also possible for multiple containers to be in the same
namespace which is what the deployment unit of kubernetes is; the pod.

In a `pid` namespace your process becomes PID 1. You can only see this process
and child processes, all others on the underlying host system are "gone".

A `net` namespace for isolating network ip/ports.

The component responsible for this work, setting the limits for cgroups, configuring
the namespaces, mounting the filesystem, and starting the process is the
responsibility of the container runtime

What about an docker image, what does it look like?  

We can use a tool named skopeo and umoci to inspect and find out more about
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
processes. This is the responsibility of a container runtime. Docker contributed
a runtime that they extracted named `runC`. There are others as well which I might
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


```console
$ docker build -t dbevenius/faas-js-example .
```
The [build](https://github.com/docker/cli/blob/master/cli/command/image/build.go)
command will call [runBuild](https://github.com/docker/cli/blob/master/cli/command/image/build.go)


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

When a image is to be run, the above information is passed to a container runtime
which might be from Docker or rkt. There is component called libcontainer which
I think is now `runC` which is a universal container runtime. 

The next part is me guessing a little but with the metadata about the container
shown above and in each of the directories (hashes) there is a layer.tar which
contains the files for that layer. The container runtime will create the
cgroup, the namespaces, and mount the union filesystem. The process resulting
from this is the container.

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

### containerd
Is a container supervisor (process monitor). It does not run the containers itself, that is done
by runC. Instread it deals with container lifecycle operations. It can do image
transfers and storage, low-level storage, network attachements.
It is indended to be embedded.

#### Kubernetes Custom Resources
Are extentions of the Kubernetes API. A resource is simply an endpoint in the
kubernetes API that stores a collection of API objects (think pods or deployments
and things like that). You can add your own resources just like them using 
custom resources. After a custom resources is installed kubectl can be used to
with it just like any other object.

So the customer resource just allows for storing and retrieving structured data,
and to have functionality you have custom controllers.

#### Controllers
Each controller is responible for a particular resource.

Controller components:
```
1) Informer/SharedInformer
Watches the current state of the resource instances and sends events to the
Workqueue. The informer gets the information about an object it sends a request
to the API server. Instread of each informater caching the objects it is interested
in multiple controllers might be interested in the same resource object. Instead
of them each caching the data/state they can share the cache among themselves,
this is what a SharedInformer does.

Resource Event Handler handles the notifications when changes occur.
type ResourceEventHandlerFuncs struct {
	AddFunc    func(obj interface{})
	UpdateFunc func(oldObj, newObj interface{})
	DeleteFunc func(obj interface{})
}

2) Workqueue
Items in this queue are taken by workers to perform work.
```

#### Custom Resource Def/Controller example
[k8s-controller](./k8s-controller) is an example of a custom resource controller
written in Rust. The goal is to understand how these work with the end goal being
able to understand how other controllers are written and how they are installed
and work. 

I'm using CodeReady Container(crc) so I'll be using some none kubernetes commands:
```
$ oc login -u kubeadmin -p e4FEb-9dxdF-9N2wH-Dj7B8 https://api.crc.testing:6443
$ oc new-project my-controller
$ kubectl create -f docs/crd.yaml
customresourcedefinition.apiextensions.k8s.io/somethings.example.nodeshift created
```
We can try to access `somthings` using:
```console
$ kubectl get something -o yaml
```
But there will not be anything in there get. We have to create something using
```console
$ kubectl create -f docs/member.yaml
something.example.nodeshift/dan created
```
Now if we again try to list the resources we will see an entry in the `items` list.
```console
$ kubectl get something -o yaml
```
And we can get all Something's using:
```console
$ kubectl get Something
$ kubectl describe Something
$ kubectl describe Something/dan
```

```console
$ kubectl config current-context
default/api-crc-testing:6443/kube:admin
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

So, we should now have a kubernetes cluster up and running. 
```console
$ minikube status
```
Show the status of the Control Plane components:
```console
$ kubectl get componentstatuses
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


### Building
Go from source to container.

There is active work to migrate Builds to Build Pipelines, a restructuring of
builds in Knative that more closely resembles CI/CD pipelines. 
This means builds in Knative, in addition to compiling and packaging your code,
can also easily run tests and publish those results.
TODO: Take a closer look at CI/CD pipelines.


How do we reach out to services that require authentication at build-time?
How do we pull code from a private Git repository or push container images to Docker Hub?

#### Secrets
Secrets allow us to securely store the credentials needed for these
authenticated requests.

#### Service Accounts.
Service Accounts allow us the flexibility of providing and maintaining
credentials for multiple Builds without manually configuring them each time we
build a new application. So just really an object with a Secret which can
be reused for multiple applications.


### Serving
Automatically scale based on load, including scaling to zero when there is no load
You deploy a prebuild image to the underlying kubernetes cluster.

Serving contains a number of components/object which are described below:

#### Configuration
This will contain a name reference to the container image to deploy. This
ref is called a Revision.
Example configuration (configuration.yaml):
```
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


### Sidecar
Is a utility container that supports the main container in a pod. Remember that
a pod is a collection of one or more containers.


### Envoy
Is composed of two parts:
1) Edge
This gives a single point of ingress (external traffic; not internal to the
service mesh).
2) Service 
This is a separate process that keeps an eye on the services. 

All of these instances form a mesh and share routing information with each other.

#### Configuration
The configuration consists of listeners and clusters.

A listener tells which port it should listen to. This can also have a filter
associated with it.

So to use Knative we need istio for the service mesh (communication between
services), we also need to be able to access the target service externally which
we use some ingress service for. 

So I need to install istio (or other service mesh) and an ingress to the
kubernetes cluster and then Knative to be able to use Knative.

### Istio
Is a service mesh.
 It is also a platform, including APIs that let it integrate into any logging
platform, or telemetry or policy system
You add Istio support to services by deploying a special sidecar proxy throughout
your environment that intercepts all network communication between microservices,
then configure and manage Istio using its control plane functionality

Istio’s traffic management model relies on the Envoy proxies that are deployed along with your services. 
All traffic that your mesh services send and receive (data plane traffic) is proxied through Envoy, making it easy to direct and control traffic around your mesh without making any changes to your services.


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
isolation you can join them using namespaces which how a pod is created. This
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

### Kubernetes Deployment


A controller is a client of Kubernetes. When Kubernetes is the client and calls
out to a remote service, it is called a Webhook. 


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
