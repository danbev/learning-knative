FROM golang

RUN apt-get update && apt-get install -y libbtrfs-dev libseccomp-dev
