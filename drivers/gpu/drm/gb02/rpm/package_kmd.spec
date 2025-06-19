Name:       %{base_name}
Version:    %{version}
Release:    %{release}
Summary:    GenBu02 drm driver on Linux, released by Sietium
Group:      Sietium
License:    Sietium
URL:        https://www.sietium.com


%description

%build

%install
mkdir -p %{buildroot}/
cp -r %{pkgworkspace}/* %{buildroot}


%files
/usr/*
/etc/*
/lib/*

%post
#!/bin/sh

KERNEL_VERSION=`uname -r`
CHECK_KYLIN=`grep -q "kylin" /etc/os-release && echo "true"`
CHECK_KYLINSEC=`grep -q "kylinsec" /etc/os-release && echo "true"`
CHECK_ARCH=`uname -a |grep -q "x86_64" && echo "true"`
GENBU_SERVICE="/lib/systemd/system/genbu.service"
# Save the target kernel version to a file, which will be used for prerm script.
uname -r > /usr/local/sietium/KERNEL_VERSION

function clean_gb_driver() {
    rm -f /lib/modules/$KERNEL_VERSION/updates/gb*.ko
    rm -f /lib/modules/$KERNEL_VERSION/kernel/kylin/gpu/sietium/genbu01/gb.*
    rm -f /lib/modules/$KERNEL_VERSION/kernel/kylin/gpu/sietium/genbu02/gb*.ko
    depmod -a
    # overwriting the initramfs file when finding the gb.ko* in initramfs file
    if [ -e "/boot/initramfs-$KERNEL_VERSION.img" ];then
        if command -v lsinitrd > /dev/null 2>&1; then
            check_initrd=`lsinitrd /boot/initramfs-$KERNEL_VERSION.img |grep gb0[12]`
            if [ -n "${check_initrd}" ]; then
                if command -v dracut > /dev/null 2>&1; then
                    dracut -f
                else
                    echo "[INSTALL INFO] Clean sietium driver from initramfs"
                    mkdir -p /opt/temp_initramfs
                    rm -rf /opt/temp_initramfs/*
                    cd /opt/temp_initramfs
                    lsinitrd --unpack /boot/initramfs-$KERNEL_VERSION.img
                    find /opt/temp_initramfs/ -name "gb.ko*" |xargs rm -rf
                    find . | cpio -o -H newc |gzip -c > initramfs-$KERNEL_VERSION.img

                    cp /opt/temp_initramfs/initramfs-$KERNEL_VERSION.img /boot/
                    rm -rf /opt/temp_initramfs
                fi
            fi
        fi
    fi
    if command -v lsinitramfs > /dev/null 2>&1; then
        CHECK_INITRD=`lsinitramfs /boot/initrd.img-$KERNEL_VERSION |grep -qi sietium && echo "true"`
        if test -n "$CHECK_INITRD"
        then
            echo "Clear sietium drv from initrd.img"
            # clean gb02 driver
            rm -f /lib/modules/`uname -r`/kernel/kylin/gpu/sietium/genbu02/gb*.ko
            sudo update-initramfs -u -k $KERNEL_VERSION
        fi
    fi
}

function install_gb_config() {
    cp /usr/local/genbu/rules/91-pulseaudio-custom.rules /usr/lib/udev/rules.d/ -rf
}

install_gb_config

# clean modules.order configure of gb01
sed -i '/updates\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.dep
sed -i '/updates\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.order
sed -i '/genbu01\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.dep
sed -i '/genbu01\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.order

clean_gb_driver

mkdir -p /lib/modules/${KERNEL_VERSION}/kernel/kylin/gpu/sietium/genbu02/
cp /usr/local/sietium/${KERNEL_VERSION}/gb*.ko \
   /lib/modules/${KERNEL_VERSION}/kernel/kylin/gpu/sietium/genbu02/
#cp /usr/local/sysconf/genbu.conf /etc/modules-load.d/genbu.conf
cp /usr/local/sysconf/blacklist-genbu.conf /etc/modprobe.d/

depmod -a

function update_sietium_pciids() {
    # export Sietium's pci.ids in system file(If exist)
    sed -n "/Sietium/,$ {
    /Sietium/ {
        :loop
        p
        n
        /\t+*/ {
            b loop
        }
    }
    }" "$1" > pci.sietium.ids.old

	# export Sietium's pci.ids in local file(If exist)
	sed -n "/Sietium/,$ {
    /Sietium/ {
        :loop
        p
        n
        /\t+*/ {
            b loop
        }
    }
    }" /usr/local/sysconf/pci.ids > pci.sietium.ids.new

    # compare file exclude white space
    if diff -B pci.sietium.ids.new pci.sietium.ids.old&>/dev/null; then
        # echo "sietium pci.ids no update required. >> ESC"
		rm pci.sietium.ids.old
		rm pci.sietium.ids.new
    else
        echo "sietium pci.ids update required. >> UPDATE"
        # remove exist Sietium's pci.ids in system pci.ids file
        sed "/8510  Sietium/,$ {
            /8510  Sietium/d
            :loop
            /\t+*/ {
                d
                b loop
            }
            :loop {
                n
                b loop
            }
        }
        " "$1" > pci.ids.system

        # append new line at end of pci.ids.system file
        if [ "$(tail -n1 pci.ids.system | wc -l)" -eq 0 ];then
            echo "" >> pci.ids.system
        fi

        # append new Sietium's pci.ids to pci.ids file
        cat pci.sietium.ids.new >> pci.ids.system
        # replace pci.ids file
        mv pci.ids.system "$1"
		rm pci.sietium.ids.old
		rm pci.sietium.ids.new
    fi
}

if [ -e /usr/share/misc/pci.ids ]; then
    update_sietium_pciids /usr/share/misc/pci.ids
    # grep -q Sietium /usr/share/misc/pci.ids || \
    #     cat /usr/local/sysconf/pci.ids >> /usr/share/misc/pci.ids
fi

if [ -e /usr/share/hwdata/pci.ids ]; then
    update_sietium_pciids /usr/share/hwdata/pci.ids
    # grep -q Sietium /usr/share/hwdata/pci.ids || \
    #     cat /usr/local/sysconf/pci.ids >> /usr/share/hwdata/pci.ids
fi

if test -n "$CHECK_KYLIN"; then
    if test -n "$CHECK_ARCH"; then
        if [ -e /boot/grub/grub.cfg ];then
            grep -q "video=efifb:on" /boot/grub/grub.cfg || \
                sed -i 's/splash/splash\ video=efifb:on/' /boot/grub/grub.cfg
        else
            # Kylin 2207 GFB Server
            if [ -e /boot/efi/EFI/kylin/grub.cfg ];then
                grep -q "video=efifb:on" /boot/efi/EFI/kylin/grub.cfg || \
                    sed -i 's/splash/splash\ video=efifb:on/' /boot/efi/EFI/kylin/grub.cfg
            fi
        fi
    else
        if [ -e /boot/efi/boot/grub/grub.cfg ];then
            grep -q "video=efifb:on" /boot/efi/boot/grub/grub.cfg || \
                sed -i 's/splash/splash\ video=efifb:on/' /boot/efi/boot/grub/grub.cfg
        else
            # Kylin 2207 GFB Server
            if [ -e /boot/efi/EFI/kylin/grub.cfg ];then
               grep -q "video=efifb:on" /boot/efi/EFI/kylin/grub.cfg || \
                   sed -i 's/splash/splash\ video=efifb:on/' /boot/efi/EFI/kylin/grub.cfg
            fi
        fi
    fi
    if [ -e $GENBU_SERVICE ];then
        systemctl enable -q $GENBU_SERVICE
    fi
    if test -n "$CHECK_KYLINSEC"; then
        if command -v dracut > /dev/null 2>&1; then
            dracut -f
        fi
    fi
fi

echo "Install Success!"

exit 0


%preun
#!/bin/sh

KERNEL_VERSION=`cat /usr/local/sietium/KERNEL_VERSION`
GENBU_SERVICE_NAME="genbu.service"
GENBU_SERVICE_PATH="/lib/systemd/system/"
#rm -f /etc/modules-load.d/genbu.conf

if [ -e $GENBU_SERVICE_PATH$GENBU_SERVICE_NAME ];then
	systemctl disable -q $GENBU_SERVICE_NAME
fi

function clean_gb_driver() {
    # clean gb01 driver
    rm -f /lib/modules/$KERNEL_VERSION/updates/gb*
    rm -f /lib/modules/$KERNEL_VERSION/kernel/kylin/gpu/sietium/genbu01/gb*

    # clean gb02 driver
    rm -f /lib/modules/$KERNEL_VERSION/kernel/kylin/gpu/sietium/genbu02/gb*.ko

    # clean module config
    sed -i '/genbu0[12]\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.dep
    sed -i '/genbu0[12]\/gb.ko/d' /lib/modules/$KERNEL_VERSION/modules.order
    depmod -a
    # overwriting the initramfs file when finding the gb.ko* in initramfs file
    if [ -e "/boot/initramfs-$KERNEL_VERSION.img" ];then
        if command -v lsinitrd > /dev/null 2>&1; then
            check_initrd=`lsinitrd /boot/initramfs-$KERNEL_VERSION.img |grep gb.ko*`
            if [ -n "${check_initrd}" ]; then
                if command -v dracut > /dev/null 2>&1; then
                    dracut -f
                fi
            fi
        fi
    fi
    if command -v lsinitramfs > /dev/null 2>&1; then
        CHECK_INITRD=`lsinitramfs /boot/initrd.img-$KERNEL_VERSION |grep -qi sietium && echo "true"`
        if test -n "$CHECK_INITRD"
        then
            echo "Clear sietium drv from initrd.img"
            # clean gb02 driver
            rm -f /lib/modules/`uname -r`/kernel/kylin/gpu/sietium/genbu02/gb*.ko
            sudo update-initramfs -u -k $KERNEL_VERSION
        fi
    fi
}

function uninstall_gb_config() {
    if [ -e /usr/lib/udev/rules.d/91-pulseaudio-custom.rules ];then
        rm /usr/lib/udev/rules.d/91-pulseaudio-custom.rules
    fi
}

clean_gb_driver

uninstall_gb_config

exit 0


%postun
#!/bin/sh

KERNEL_VERSION=`cat /usr/local/sietium/KERNEL_VERSION`

# There was a rm warning. So we check firstly.
if [ -e /etc/modprobe.d/blacklist-genbu.conf ]; then
    rm -f /etc/modprobe.d/blacklist-genbu.conf
fi
if [ -e /usr/local/sietium/$KERNEL_VERSION/gb*.ko ]; then
    rm -f /usr/local/sietium/$KERNEL_VERSION/gb*.ko
fi

rm -f /usr/local/sietium/KERNEL_VERSION
rm -f /usr/local/sietium/PACKAGE_KERNEL_VERSION

echo "Remove Success!"
exit 0

%changelog
