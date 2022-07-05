EXTERNAL_IP=$(kubectl -n kourier-system get service kourier -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
#export EXTERNAL_IP=$(minikube -n kourier-system  service kourier --url=true | sed -e 's_http://\([0-9.]*\):\([0-9]*\)_\1_' | head -1)
echo EXTERNAL_IP=$EXTERNAL_IP
