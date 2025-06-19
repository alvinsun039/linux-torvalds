#!/bin/bash

gb_module="gb02.ko"
module_version=$1

if [ -L $gb_module ]; then
	exit 0
fi

if [ -f $gb_module -a ! -f $module_version ]; then
	mv $gb_module $module_version
	ln -s $module_version $gb_module
fi
