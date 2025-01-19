#!/usr/bin/env bash
set -euxo pipefail

cc cube.c -Wall -Wextra -ggdb -std=c11 -pedantic -o out -lraylib -lm
