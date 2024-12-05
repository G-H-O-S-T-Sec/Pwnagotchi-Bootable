#!/bin/bash

# Exit on error
set -e

echo "Optimizing Raspberry Pi Zero W for Anon..."

# Optimize boot configuration
echo "Optimizing boot configuration..."
mount /dev/mmcblk0p1 /boot

cat > /boot/config.txt << EOL
# Minimize power usage
arm_freq=1000
gpu_freq=200
core_freq=250
sdram_freq=400
over_voltage=-2

# Disable unused features
dtparam=audio=off
dtoverlay=disable-bt
dtparam=act_led_trigger=none
dtparam=act_led_activelow=off
dtparam=pwr_led_trigger=none
dtparam=pwr_led_activelow=off

# Minimize boot time
boot_delay=0
initial_turbo=1

# Disable HDMI to save power
hdmi_blanking=2
hdmi_force_hotplug=0
hdmi_drive=1

# GPU memory split (minimum for headless)
gpu_mem=16

# Enable hardware crypto if available
dtoverlay=crypto

# Temperature control
temp_limit=70
temp_soft_limit=60
EOL

# Optimize kernel parameters
cat > /boot/cmdline.txt << EOL
console=serial0,115200 console=tty1 root=PARTUUID=738a4d67-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait quiet loglevel=3 logo.nologo vt.global_cursor_default=0 plymouth.ignore-serial-consoles fastboot noatime nodiratime noswap
EOL

# Optimize system services
echo "Optimizing system services..."
systemctl disable bluetooth
systemctl disable avahi-daemon
systemctl disable triggerhappy
systemctl disable apt-daily.timer
systemctl disable apt-daily-upgrade.timer
systemctl disable man-db.timer
systemctl disable dphys-swapfile.service

# Optimize memory usage
echo "Optimizing memory usage..."
cat > /etc/sysctl.d/99-memory.conf << EOL
vm.swappiness=10
vm.dirty_ratio=20
vm.dirty_background_ratio=10
vm.vfs_cache_pressure=50
EOL

# Optimize network settings
echo "Optimizing network settings..."
cat > /etc/sysctl.d/99-network.conf << EOL
net.core.rmem_max=2097152
net.core.wmem_max=2097152
net.ipv4.tcp_rmem=4096 87380 2097152
net.ipv4.tcp_wmem=4096 87380 2097152
net.ipv4.tcp_congestion_control=bbr
net.core.netdev_budget=600
net.core.netdev_budget_usecs=8000
EOL

# Optimize filesystem
echo "Optimizing filesystem..."
cat > /etc/fstab << EOL
PARTUUID=738a4d67-01  /boot           vfat    defaults,noatime  0       2
PARTUUID=738a4d67-02  /               ext4    defaults,noatime,nodiratime,commit=60  0       1
tmpfs                 /tmp            tmpfs   defaults,noatime,mode=1777  0       0
tmpfs                 /var/log        tmpfs   defaults,noatime,mode=0755  0       0
tmpfs                 /var/tmp        tmpfs   defaults,noatime,mode=1777  0       0
EOL

# Create RAM disk for temporary files
mkdir -p /var/log/anon
mount -t tmpfs -o size=50M,mode=0755 tmpfs /var/log/anon

# Optimize WiFi settings
echo "Optimizing WiFi settings..."
cat > /etc/modprobe.d/wifi-optimize.conf << EOL
options cfg80211 ieee80211_regdom=00
options mac80211 max_nullfunc_tries=1
options mac80211 probe_wait_ms=70
options mac80211 beacon_loss_count=2
EOL

# Set up CPU governor
echo "Optimizing CPU settings..."
cat > /etc/init.d/cpu_governor << EOL
#!/bin/sh
### BEGIN INIT INFO
# Provides:          cpu_governor
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Set CPU governor
### END INIT INFO

case "\$1" in
  start)
    echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
    echo 50 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
    echo 100000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
    ;;
  *)
    echo "Usage: /etc/init.d/cpu_governor {start}"
    exit 1
    ;;
esac
exit 0
EOL

chmod +x /etc/init.d/cpu_governor
update-rc.d cpu_governor defaults

# Create cleanup script
cat > /usr/local/sbin/anon_cleanup << EOL
#!/bin/bash
# Clean logs
find /var/log/anon -type f -mtime +1 -delete
# Clean old handshakes
find /opt/anon/handshakes -type f -mtime +7 -delete
# Clean temporary files
rm -rf /tmp/*
EOL
chmod +x /usr/local/sbin/anon_cleanup

# Add cleanup to cron
(crontab -l 2>/dev/null; echo "0 3 * * * /usr/local/sbin/anon_cleanup") | crontab -

echo "Optimization complete! Please reboot for changes to take effect."
