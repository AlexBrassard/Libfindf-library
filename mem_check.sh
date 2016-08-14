#!/bin/bash

# Simple hard coded script to run valgrind with some options.
valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./public_regex_test

