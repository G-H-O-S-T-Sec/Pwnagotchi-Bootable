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
BOOT_SIZE=256 # MB
ROOT_SIZE=1024 # MB

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

# Unmount any existing partitions
umount ${SD_CARD}* 2>/dev/null || true

# Create new partition table
parted -s $SD_CARD mklabel msdos

# Create boot partition (FAT32)
parted -s $SD_CARD mkpart primary fat32 1MiB ${BOOT_SIZE}MiB
parted -s $SD_CARD set 1 boot on

# Create root partition (ext4)
parted -s $SD_CARD mkpart primary ext4 ${BOOT_SIZE}MiB $((BOOT_SIZE + ROOT_SIZE))MiB

# Format partitions
mkfs.vfat -F 32 ${SD_CARD}1
mkfs.ext4 -F ${SD_CARD}2

# Create mount points
mkdir -p /mnt/boot
mkdir -p /mnt/rootfs

# Mount partitions
mount ${SD_CARD}1 /mnt/boot
mount ${SD_CARD}2 /mnt/rootfs

# Download and extract Raspberry Pi OS Lite
RASPBIAN_URL="https://downloads.raspberrypi.org/raspios_lite_armhf_latest"
wget -O raspbian.zip $RASPBIAN_URL
unzip raspbian.zip
RASPBIAN_IMG=$(ls *raspios*.img)

# Mount the Raspberry Pi OS image
LOOP_DEVICE=$(losetup -f)
losetup -P $LOOP_DEVICE $RASPBIAN_IMG

# Copy boot files
cp -r ${LOOP_DEVICE}p1/* /mnt/boot/

# Copy root filesystem
cp -r ${LOOP_DEVICE}p2/* /mnt/rootfs/

# Configure for headless operation
touch /mnt/boot/ssh
cat > /mnt/boot/wpa_supplicant.conf << EOL
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="WIFI_NAME"
    psk="WIFI_PASSWORD"
    key_mgmt=WPA-PSK
}
EOL

# Create startup service
cat > /mnt/rootfs/etc/systemd/system/stealth.service << EOL
[Unit]
Description=Stealth System Service
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/stealth
ExecStart=/opt/stealth/stealth_system
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOL

# Create installation directory
mkdir -p /mnt/rootfs/opt/stealth

# Cleanup
sync
umount /mnt/boot
umount /mnt/rootfs
losetup -d $LOOP_DEVICE
rm raspbian.zip $RASPBIAN_IMG

echo "SD card preparation complete!"
