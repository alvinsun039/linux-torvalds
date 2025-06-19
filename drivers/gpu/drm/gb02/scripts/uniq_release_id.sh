#!/bin/bash

. /etc/os-release;KERN=`uname -r`;echo $KERN|grep -q `arch`||ARCH_SUFFIX=-`arch`; MinorVersion=`(grep -s ^MinorVersion= /etc/os-version||echo $KYLIN_RELEASE_ID)|sed -e 's/[^0-9.]//g'`;OsBuild=`grep -s ^OsBuild= /etc/os-version|sed -e 's/[^0-9.]//g' -e 's/^/-/'`; UNIQ_RELEASE_ID="$VERSION_ID$ID$MinorVersion$OsBuild-$KERN$ARCH_SUFFIX";echo ${UNIQ_RELEASE_ID}
