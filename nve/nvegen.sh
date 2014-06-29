#!/bin/bash


# use for CONFIG_BOARD_IS_HUAWEI_PXA920


cc NVE_partition.c -o nve_gen.o

./nve_gen.o $1

rm -rf nve_gen.o

