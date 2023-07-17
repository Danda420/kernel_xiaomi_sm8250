#!/bin/bash
export KERNEL_DIR=$PWD
cd ..
MAIN=$PWD
cd $KERNEL_DIR
export ANYKERNEL_DIR=$KERNEL_DIR/AnyKernel3
export ARCH="arm64"
export CLANG_DIR="$MAIN/clang"
export CC="${CLANG_DIR}/bin/clang"
export LINKER="lld"
export compiler_name="Oxygen"
export DTSI_DIR=$KERNEL_DIR/arch/arm64/boot/dts/vendor/qcom
export APOLLO=$DTSI_DIR/dsi-panel-j3s-37-02-0a-dsc-video.dtsi
export ALIOTH=$DTSI_DIR/dsi-panel-k11a-38-08-0a-dsc-cmd.dtsi
export MUNCH=$DTSI_DIR/dsi-panel-l11r-38-08-0a-dsc-cmd.dtsi
KERNELVER="$(make kernelversion)"
DATE_CLOCK="$(date +'%H%M-%d%m%Y')"

aosp_build() {
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <695>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1544>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <700>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1540>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <695>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $MUNCH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1546>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $MUNCH
}

restore_dtsi_dimension() {
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <70>;/qcom,mdss-pan-physical-width-dimension = <695>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <155>;/qcom,mdss-pan-physical-height-dimension = <1544>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <70>;/qcom,mdss-pan-physical-width-dimension = <700>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <155>;/qcom,mdss-pan-physical-height-dimension = <1540>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <70>;/qcom,mdss-pan-physical-width-dimension = <695>;/g' $MUNCH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <155>;/qcom,mdss-pan-physical-height-dimension = <1546>;/g' $MUNCH
}

build_non_ksu() {
	sed -i 's/CONFIG_KSU=y/# CONFIG_KSU is not set/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	sed -i 's/CONFIG_OVERLAY_FS_REDIRECT_DIR=y/# CONFIG_OVERLAY_FS_REDIRECT_DIR is not set/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	sed -i 's/CONFIG_OVERLAY_FS_INDEX=y/# CONFIG_OVERLAY_FS_INDEX is not set/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	ZIPNAME=$KERNELVER-$DEVICE-${BUILD_TYPE}${VARIANT}Oxygen+-$DATE_CLOCK.zip
}

build_ksu() {
	sed -i 's/# CONFIG_KSU is not set/CONFIG_KSU=y/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	sed -i 's/# CONFIG_OVERLAY_FS_REDIRECT_DIR is not set/CONFIG_OVERLAY_FS_REDIRECT_DIR=y/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	sed -i 's/# CONFIG_OVERLAY_FS_INDEX is not set/CONFIG_OVERLAY_FS_INDEX=y/g' $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	ZIPNAME=$KERNELVER-$DEVICE-${BUILD_TYPE}${VARIANT}Oxygen+-KernelSU-$DATE_CLOCK.zip
}

build_oc() {
	mkdir $ANYKERNEL_DIR/oxygen+/backup
	mv arch/arm64/boot/dts/vendor/qcom/kona-v2-gpu.dtsi $ANYKERNEL_DIR/oxygen+/backup/
	cp $ANYKERNEL_DIR/oxygen+/kona-v2-gpu.dtsi arch/arm64/boot/dts/vendor/qcom/kona-v2-gpu.dtsi
	chmod 755 arch/arm64/boot/dts/vendor/qcom/kona-v2-gpu.dtsi
}

restore_gpu_dtsi() {
	mv $ANYKERNEL_DIR/oxygen+/backup/kona-v2-gpu.dtsi arch/arm64/boot/dts/vendor/qcom/kona-v2-gpu.dtsi > /dev/null 2>&1 
	chmod 755 arch/arm64/boot/dts/vendor/qcom/kona-v2-gpu.dtsi
	rm -rf $ANYKERNEL_DIR/oxygen+/backup
}

zipkernel() {
	if [[ -f $KERNEL_DIR/out/arch/arm64/boot/Image.gz ]] && [[ -f $KERNEL_DIR/out/arch/arm64/boot/dtb.img ]] && [[ -f $KERNEL_DIR/out/arch/arm64/boot/dtbo.img ]]; then

		mv $KERNEL_DIR/out/arch/arm64/boot/Image.gz $ANYKERNEL_DIR/
		mv $KERNEL_DIR/out/arch/arm64/boot/dtb.img $ANYKERNEL_DIR/dtb
		mv $KERNEL_DIR/out/arch/arm64/boot/dtbo.img $ANYKERNEL_DIR/
		cd $ANYKERNEL_DIR
		echo ""
		echo "zipping"
		if [[ $DEVICE == apollo ]]; then
			if [[ ! -d $ANYKERNEL_DIR/ramdisk ]]; then
				mkdir $ANYKERNEL_DIR/ramdisk
				cp -r $ANYKERNEL_DIR/oxygen+/overlay.d $ANYKERNEL_DIR/ramdisk/
			fi
			cp $ANYKERNEL_DIR/oxygen+/apollo $ANYKERNEL_DIR/anykernel.sh
		elif [[ $DEVICE == alioth ]]; then
			rm -rf $ANYKERNEL_DIR/ramdisk
			cp $ANYKERNEL_DIR/oxygen+/alioth $ANYKERNEL_DIR/anykernel.sh
		elif [[ $DEVICE == munch ]]; then
			rm -rf $ANYKERNEL_DIR/ramdisk
			cp $ANYKERNEL_DIR/oxygen+/munch $ANYKERNEL_DIR/anykernel.sh
		fi
		zip -r9 $ZIPNAME META-INF ramdisk tools anykernel.sh banner Image.gz dtb dtbo.img
		mv $ZIPNAME $KERNEL_DIR/
		echo ""

		echo "Cleaning AnyKernel3 dir"
		echo ""
		rm -rf $ANYKERNEL_DIR/*.zip
		rm -rf $ANYKERNEL_DIR/Image.gz
		rm -rf $ANYKERNEL_DIR/dtb
		rm -rf $ANYKERNEL_DIR/dtbo.img
		echo "****** build done! ******"
		echo ""
		echo "Your kernel zip name is $ZIPNAME"
		echo ""
		BUILD_END=$(date +"%s")
		DIFF=$(($BUILD_END - $BUILD_START))
		echo -e "Build completed in '$(($DIFF / 60))' minute(s) and '$(($DIFF % 60))' seconds"
	else
		echo " Kernel build failed! abort zipping..."
	fi
	rm -rf $KERNEL_DIR/out/
}

build() {
	DEFCONFIG=vendor/${DEVICE}_defconfig
	echo "cleaning..."
	rm -rf $KERNEL_DIR/out/
	mkdir -p out
	clear
	BUILD_START=$(date +"%s")
	export USE_CCACHE=1
	export CCACHE_EXEC=/usr/bin/ccache
	MAKE="./makeparallel"
	ccache -M 10G
	ccache -o compression=true

	export PATH="${CLANG_DIR}/bin:${PATH}"
		
	get_compiler_info="$($CLANG_DIR/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')"
	get_name=${get_compiler_info%% *}
	if [[ -z $compiler_name ]]; then
	   set_compiler=$get_compiler_info
	else
	   set_compiler=$(echo $get_compiler_info | sed "s/$get_name/$compiler_name/g")
	fi;
	export KBUILD_COMPILER_STRING="$set_compiler"
	
	make O=out ARCH=${ARCH} $DEFCONFIG \
			ARCH=${ARCH} \
			LLVM=1 \
			LLVM_IAS=1 \
			CC=${CC} \
			CROSS_COMPILE=${CLANG_DIR}/bin/aarch64-linux-gnu- \
			CROSS_COMPILE_COMPAT=${CLANG_DIR}/bin/arm-linux-gnueabi- \
			LD_LIBRARY_PATH=${CLANG_DIR}/lib \
			    
		make O=out \
			ARCH=${ARCH} \
			LLVM=1 \
			LLVM_IAS=1 \
			CC=${CC} \
			CLANG_TRIPLE=${CLANG_DIR}/bin/aarch64-linux-gnu- \
			CROSS_COMPILE=${CLANG_DIR}/bin/aarch64-linux-gnu- \
			CROSS_COMPILE_COMPAT=${CLANG_DIR}/bin/arm-linux-gnueabi- \
			LD=ld.${LINKER} \
			AR=llvm-ar \
			NM=llvm-nm \
			OBJCOPY=llvm-objcopy \
			OBJDUMP=llvm-objdump \
			STRIP=llvm-strip \
			ld-name=${LINKER} \
			-j$(nproc --all) CC="ccache ${CC}"
			
		restore_gpu_dtsi
		zipkernel
		restore_dtsi_dimension
		build_non_ksu
	}

choose_build() {
	echo ""
	echo "****** Building kernel for $DEVICE ******"
	echo ""
	while true; do
	    echo " "
	    echo "1. Build kernel without KernelSU"
	    echo "2. Build kernel with KernelSU"
	    read -p "" choose
	    case $choose in
		1 ) 
		build_non_ksu
		build
		exit;;
		2 )
		build_ksu
		build
		exit;;
		* ) 
		echo "Please answer 1 or 2.";;
	    esac
	done

}

if [[ $1 == oc ]]; then
	build_oc
	VARIANT=OC-
fi

if [[ $2 == aosp ]]; then
	aosp_build
	BUILD_TYPE=AOSP-
fi

clear 
while true; do
    echo "Build kernel for :"
    echo "(1) MI 10T/Pro (apollo)"
    echo "(2) POCO F3 (alioth)"
    echo "(3) POCO F4 (munch)"
    read -p "" choose
    case $choose in
        1 ) 
        DEVICE=apollo
        choose_build
        exit;;
        2 )
        DEVICE=alioth
        choose_build
        exit;;
        3 )
        DEVICE=munch
        choose_build
        exit;;
        * ) 
        echo "Please answer 1, 2 or 3.";;
    esac
done
