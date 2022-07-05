export KNATIVE_NET_KOURIER_VERSION="1.0.0"

kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v${KNATIVE_NET_KOURIER_VERSION}/kourier.yaml
kubectl wait pod --timeout=-1s --for=condition=Ready -l '!job-name' -n kourier-system
kubectl wait pod --timeout=-1s --for=condition=Ready -l '!job-name' -n knative-serving
