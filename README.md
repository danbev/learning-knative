Knative looks to build on Kubernetes and present a consistent, standard pattern
for building and deploying serverless and event-driven applications. 


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



### Servicing
Automatically scale based on load, including scaling to zero when there is no load
You deploy a prebuild image to the underlying kubernetes cluster.

#### Configuration
This will contain a name reference to the container image to deploy. This
ref is called a Revision.

#### Revision
Immutable snapshots of code and configuration. Refs a specific container image
to run. Knative creates Revisions for us when we modify the Configuration.
Since Revisions are immutable and multiple versions can be running at once,
it’s possible to bring up a new Revision while serving traffic to the old version.
Then, once you are ready to direct traffic to the new Revision, update the Route
to instantly switch over. This is sometimes referred to as a blue-green
deployment, with blue and green representing the different versions.

#### Route

#### Service


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
Channels are an abstraction between our code and the underlying messaging
solution. This means we could swap this between something like Kafka and RabbitMQ

#### Subscriptions
Subscriptions are the glue between Channels and Services, instructing Knative
how our events should be piped through the entire system.



### Istio
Istio is a service mesh that provides many useful features on top of Kubernetes
including traffic management, network policy enforcement, and observability. We
don’t consider Istio to be a component of Knative but instead one of its
dependencies, just as Kubernetes is. Knative ultimately runs atop a Kubernetes
cluster with Istio.


### Operators
In OpenShift Operators are the preferred method of packaging, deploying, and managing services on the control plane. 

### Kubernetes

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

#### Worker nodes
These run the containers and provide the runtime. A worker node is comprised of
a kublet. It watches the API Server for pods that have been assigned to it.
Inside each pod there are containers. Kublet runs these via Docker by pulling
images, stopping, starting, etc.
Another part of a worker node is the kube-proxy which maintains networking rules
on the host and performing connection forwarding. It also takes care of load
balancing.

#### Kubernetes Pod
Is a group of one or more containers with shared storage and network. Pods are
the unit of scaling.


### API Groups
Kubernetes uses a versioned API which is categoried into API groups. For example,
you might find:
```
apiVersion: serving.knative.dev/v1alpha1
```
in a yaml file which is specifying the API group, `serving.knative.dev` and the
version.

