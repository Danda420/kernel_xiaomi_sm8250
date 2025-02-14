#!/bin/bash


DEVICE=$1
KERNEL_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
DTSI_DIR=$KERNEL_DIR/arch/arm64/boot/dts/vendor/qcom
APOLLO=$DTSI_DIR/dsi-panel-j3s-37-02-0a-dsc-video.dtsi
ALIOTH=$DTSI_DIR/dsi-panel-k11a-38-08-0a-dsc-cmd.dtsi
MUNCH=$DTSI_DIR/dsi-panel-l11r-38-08-0a-dsc-cmd.dtsi

aosp_build() {
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <695>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1544>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $APOLLO
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <695>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1546>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $ALIOTH
	sed -i 's/qcom,mdss-pan-physical-width-dimension = <695>;/qcom,mdss-pan-physical-width-dimension = <70>;/g' $MUNCH
	sed -i 's/qcom,mdss-pan-physical-height-dimension = <1546>;/qcom,mdss-pan-physical-height-dimension = <155>;/g' $MUNCH
	sed -i "s/CONFIG_OPLUS=y/# CONFIG_OPLUS is not set/g" $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
	sed -i "s/CONFIG_OPLUS_FEATURE_EROFS=y/# CONFIG_OPLUS_FEATURE_EROFS is not set/g" $KERNEL_DIR/arch/arm64/configs/vendor/${DEVICE}_defconfig
}


aosp_build 