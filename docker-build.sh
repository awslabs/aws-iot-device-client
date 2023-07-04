#!/bin/bash
# Defaults to ubuntu (18.04). Other options are amazonlinux and ubi8
os=${1:-"ubuntu"}

docker build . --file .github/docker-images/base-images/device-client/"$os"/Dockerfile
