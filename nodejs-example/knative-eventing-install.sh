export KNATIVE_EVENTING_VERSION="1.0.0"
kubectl apply --filename https://github.com/knative/eventing/releases/download/knative-v${KNATIVE_EVENTING_VERSION}/eventing-crds.yaml
kubectl wait --for=condition=Established --all crd

kubectl apply --filename https://github.com/knative/eventing/releases/download/knative-v${KNATIVE_EVENTING_VERSION}/eventing-core.yaml

kubectl wait pod --timeout=-1s --for=condition=Ready -l '!job-name' -n knative-eventing

kubectl apply --filename https://github.com/knative/eventing/releases/download/knative-v${KNATIVE_EVENTING_VERSION}/in-memory-channel.yaml

kubectl wait pod --timeout=-1s --for=condition=Ready -l '!job-name' -n knative-eventing

kubectl apply --filename https://github.com/knative/eventing/releases/download/knative-v${KNATIVE_EVENTING_VERSION}/mt-channel-broker.yaml

kubectl wait pod --timeout=-1s --for=condition=Ready -l '!job-name' -n knative-eventing

