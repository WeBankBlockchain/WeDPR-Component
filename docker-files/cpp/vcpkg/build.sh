#!/bin/bash
echo "* build image: wedpr-component-vcpkg-cache, branch: ${1}"
docker build --build-arg SOURCE_BRANCH=${1} -t wedpr-component-vcpkg-cache .
echo "* build image: wedpr-component-vcpkg-cache success, branch: ${1}"
