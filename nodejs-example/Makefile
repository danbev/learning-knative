CONTAINER_NAME="node_container"

build:
	docker build -t node-example .

run: 
	docker run --name $(CONTAINER_NAME) -p 8080:8080 -d node-example

run-shell:
	docker run -it --entrypoint sh  node-example

exec:
	docker exec -it $(CONTAINER_NAME) bash

ps: 
	docker ps -f name=$(CONTAINER_NAME)

kill:
	docker kill $(CONTAINER_NAME)
	docker rm $(CONTAINER_NAME)

inspect:
	docker inspect $(CONTAINER_NAME)

curl:
	curl localhost:8080

publish:
	docker tag node-example dbevenius/node-example
	docker push dbevenius/node-example

minikube:
	minikube start

info:
	kubectl cluster-info

krun:
	kubectl run node-example --image=dbevenius/node-example --port=8080 --generator=run-pod/v1

expose:
	kubectl expose rc node-example --type=LoadBalancer --name node-http

services:
	kubectl get svc

minikube-services:
	minikube service node-http --url=true

pods: 
	kubectl get pods
