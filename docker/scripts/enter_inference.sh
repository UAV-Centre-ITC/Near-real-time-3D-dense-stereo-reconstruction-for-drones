#!/bin/bash
exec docker exec -it -u "$(id -u):$(id -g)" s2m2-inference bash
