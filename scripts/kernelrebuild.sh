#!/bin/sh
./jinx rebuild kernel
make
make run-kvm