#!/bin/bash

# Pwnagotchi Boot Configuration Script
# This script sets up the necessary configuration for booting from SD card

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

# Base directories
PWNAGOTCHI_ROOT="/opt/pwnagotchi"
SD_CARD_PATH="/dev/mmcblk0"
MOUNT_POINT="/mnt/pwnagotchi"

# Create necessary directories
mkdir -p "${PWNAGOTCHI_ROOT}"/{config,captures,logs,models,plugins}
mkdir -p "${MOUNT_POINT}"

# Function to setup storage partitions
setup_storage() {
    echo "Setting up storage partitions..."
    
    # Create two partitions: boot (256MB) and root (rest)
    parted -s "${SD_CARD_PATH}" mklabel msdos
    parted -s "${SD_CARD_PATH}" mkpart primary fat32 1MiB 257MiB
    parted -s "${SD_CARD_PATH}" mkpart primary ext4 257MiB 100%
    
    # Format partitions
    mkfs.vfat "${SD_CARD_PATH}p1"
    mkfs.ext4 "${SD_CARD_PATH}p2"
    
    # Mount root partition
    mount "${SD_CARD_PATH}p2" "${MOUNT_POINT}"
}

# Function to configure boot parameters
configure_boot() {
    echo "Configuring boot parameters..."
    
    # Create boot configuration
    cat > "${MOUNT_POINT}/boot/config.txt" << EOF
# Pwnagotchi Boot Configuration

# Display settings
hdmi_force_hotplug=1
hdmi_group=2
hdmi_mode=87
hdmi_cvt=800 480 60

# Power settings
initial_turbo=30
force_turbo=0
arm_freq=1000
gpu_freq=500
core_freq=500
sdram_freq=500
over_voltage=0

# Memory split
gpu_mem=16

# Enable I2C and SPI
dtparam=i2c_arm=on
dtparam=spi=on

# Enable audio (if needed)
dtparam=audio=on

# Enable USB OTG
dtoverlay=dwc2
EOF

    # Create cmdline.txt
    echo "console=serial0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait quiet init=/usr/lib/raspi-config/init_resize.sh" > "${MOUNT_POINT}/boot/cmdline.txt"
}

# Function to setup system services
setup_services() {
    echo "Setting up system services..."
    
    # Create systemd service for pwnagotchi
    cat > "/etc/systemd/system/pwnagotchi.service" << EOF
[Unit]
Description=Pwnagotchi AI
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=${PWNAGOTCHI_ROOT}
ExecStart=${PWNAGOTCHI_ROOT}/pwnagotchi
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

    # Enable service
    systemctl enable pwnagotchi.service
}

# Function to optimize system for SD card
optimize_system() {
    echo "Optimizing system for SD card usage..."
    
    # Configure fstab for optimized SD card access
    cat > "${MOUNT_POINT}/etc/fstab" << EOF
/dev/mmcblk0p1  /boot           vfat    defaults,noatime  0       2
/dev/mmcblk0p2  /              ext4    defaults,noatime  0       1
tmpfs           /tmp            tmpfs   defaults,noatime,mode=1777  0       0
tmpfs           /var/log        tmpfs   defaults,noatime,mode=0755  0       0
tmpfs           /var/tmp        tmpfs   defaults,noatime,mode=1777  0       0
EOF

    # Set up log rotation
    cat > "${MOUNT_POINT}/etc/logrotate.d/pwnagotchi" << EOF
${PWNAGOTCHI_ROOT}/logs/*.log {
    rotate 5
    weekly
    maxsize 50M
    missingok
    notifempty
    compress
    delaycompress
    copytruncate
}
EOF
}

# Main installation process
echo "Starting Pwnagotchi installation..."

# 1. Setup storage
setup_storage

# 2. Configure boot parameters
configure_boot

# 3. Setup services
setup_services

# 4. Optimize system
optimize_system

# 5. Copy necessary files
cp pwnagotchi "${PWNAGOTCHI_ROOT}/"
chmod +x "${PWNAGOTCHI_ROOT}/pwnagotchi"

echo "Installation complete!"
echo "Please reboot your system to start Pwnagotchi."
