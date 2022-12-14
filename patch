#!/bin/bash

if [ -f $PWD/*.patch ]; then
	clear
	echo " ";
	echo -e "**** Patching kernel ****";
	echo " ";
	patch -N -p1 < *.patch
	rm -f $PWD/*.patch
	#clear
	echo " ";
	echo -e "**** Patching kernel finished ****";
else
	clear
	echo " ";
	echo -e "**** Patch kernel no found ****";
	echo " ";
	echo "Make patch file from diff, and place into Kernel dir ";
fi
