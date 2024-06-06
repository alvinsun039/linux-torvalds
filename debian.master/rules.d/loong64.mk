human_arch      = loongarch64
build_arch      = loongarch
header_arch     = loongarch
defconfig       = defconfig
flavours        = generic
build_image     = vmlinux vmlinuz.efi
kernel_file     = vmlinux
install_file    = vmlinuz
no_dumpfile     = true
elf_signed 	= true

do_tools_usbip  = true
do_tools_cpupower = true
do_tools_perf   = true
do_extras_package = true
do_cloud_tools  = true
do_libc_dev_package = true
do_doc_package = true
