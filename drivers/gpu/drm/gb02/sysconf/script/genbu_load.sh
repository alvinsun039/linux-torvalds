#! /usr/bin/env bash

CHECK_GENBU=`lspci -n|grep 8510`
if test -n "$CHECK_GENBU"
then
    modprobe drm
    modprobe drm-kms-helper
    modprobe gpu-sched
    modprobe ttm
    modprobe gb02
fi
