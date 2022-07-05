## Docker/Kubernetes example

### Docker
#### Building image
```console
$ make build
```

#### Running in docker
```console
$ make run
```

#### Kill container
```console
$ make kill
```

#### Access/call
```console
$ curl localhost:8080

### Kubernetes (minikube)

#### Start minikube
```console
$ make minikube
```

#### Deploy pod
```console
$ make krun
```

### Get ip/port:
```console
$ make minikube-service
```

### Access/call
Use the ip/port from the previous set.
```console
$ curl http://192.168.49.2:30847
Container id (os.hostname): node-example-xklkb
```


