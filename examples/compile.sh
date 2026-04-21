#!/bin/bash

./compiler $1
clang output.ll -o $2
