build:
	docker build -t node-example .

run: 
	docker run --name node-container -p 8080:8080 -d node-example

run-shell:
	docker run -it --entrypoint sh  node-example

ps: 
	docker ps -f name=node-container

kill:
	docker kill node-container
