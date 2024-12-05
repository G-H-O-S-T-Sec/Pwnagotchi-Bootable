#!/bin/bash

# Exit on error
set -e

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root"
    exit 1
fi

# Configuration
SD_CARD=$1
BUILD_DIR="build"
TEMP_DIR="/tmp/anon_build"
MOUNT_POINT="/mnt"

if [ -z "$SD_CARD" ]; then
    echo "Usage: $0 /dev/sdX (replace X with your SD card device letter)"
    exit 1
fi

echo "WARNING: This will erase all data on $SD_CARD"
read -p "Are you sure you want to continue? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

# Create temporary directory
mkdir -p $TEMP_DIR

# Download Raspberry Pi OS Lite
echo "Downloading Raspberry Pi OS Lite..."
wget -O $TEMP_DIR/raspios.zip https://downloads.raspberrypi.org/raspios_lite_armhf_latest
unzip $TEMP_DIR/raspios.zip -d $TEMP_DIR
RASPIOS_IMG=$(ls $TEMP_DIR/*raspios*.img)

# Flash OS to SD card
echo "Flashing Raspberry Pi OS to SD card..."
dd if=$RASPIOS_IMG of=$SD_CARD bs=4M status=progress conv=fsync

# Wait for the OS to finish writing
sync

# Mount partitions
echo "Mounting partitions..."
mkdir -p /mnt/boot
mkdir -p /mnt/rootfs
mount ${SD_CARD}1 /mnt/boot
mount ${SD_CARD}2 /mnt/rootfs

# Enable SSH
touch /mnt/boot/ssh

# Configure WiFi (optional)
cat > /mnt/boot/wpa_supplicant.conf << EOL
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="ROBHOME"
    psk="Wefly@21"
    key_mgmt=WPA-PSK
}
EOL

# Copy optimization scripts
cp build_scripts/optimize.sh /mnt/rootfs/root/
cp build_scripts/install.sh /mnt/rootfs/root/

# Copy setup scripts to boot partition
mkdir -p "${MOUNT_POINT}/boot/setup"
cp setup_pwnagotchi.sh "${MOUNT_POINT}/boot/setup/"
chmod +x "${MOUNT_POINT}/boot/setup/setup_pwnagotchi.sh"

# Create first boot script
cat > /mnt/rootfs/root/first_boot.sh << EOL
#!/bin/bash

# Update system
apt-get update
apt-get upgrade -y

# Run installation and optimization
chmod +x /root/install.sh
chmod +x /root/optimize.sh
/root/install.sh
/root/optimize.sh

# Clean up
rm /root/first_boot.sh
rm /root/install.sh
rm /root/optimize.sh

# Reboot
reboot
EOL

chmod +x /mnt/rootfs/root/first_boot.sh

# Add first boot script to rc.local
sed -i 's/exit 0//' /mnt/rootfs/etc/rc.local
echo "/root/first_boot.sh" >> /mnt/rootfs/etc/rc.local
echo "exit 0" >> /mnt/rootfs/etc/rc.local

# Add setup script to rc.local to run on first boot
cat > "${MOUNT_POINT}/etc/rc.local" << EOL
#!/bin/sh -e
if [ -f /boot/setup/setup_pwnagotchi.sh ]; then
    /boot/setup/setup_pwnagotchi.sh
    rm -f /boot/setup/setup_pwnagotchi.sh
fi
exit 0
EOL
chmod +x "${MOUNT_POINT}/etc/rc.local"

# Copy source files
mkdir -p /mnt/rootfs/opt/anon
cp -r * /mnt/rootfs/opt/anon/

# Create build directory
mkdir -p /mnt/rootfs/opt/anon/build

# Configure hostname
echo "anon" > /mnt/rootfs/etc/hostname
sed -i 's/raspberrypi/anon/g' /mnt/rootfs/etc/hosts

# Clean up
sync
umount /mnt/boot
umount /mnt/rootfs
rm -rf $TEMP_DIR

echo "Build complete! Insert the SD card into your Pi Zero W and power it on."
echo "The system will automatically install and configure itself on first boot."
echo "This process may take 15-20 minutes. You can monitor the progress by connecting via SSH:"
echo "ssh pi@anon.local (default password: raspberry)"
