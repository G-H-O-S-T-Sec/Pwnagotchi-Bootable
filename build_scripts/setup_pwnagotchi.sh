#!/bin/bash

# Exit on error
set -e

# Update and install dependencies
apt-get update
apt-get install -y \
    git \
    python3 \
    python3-pip \
    python3-dev \
    python3-numpy \
    python3-scipy \
    python3-pandas \
    python3-scapy \
    python3-pil \
    python3-requests \
    libpcap-dev \
    libglib2.0-dev \
    libatlas-base-dev \
    libopenjp2-7 \
    libtiff5 \
    tcpdump \
    lsof \
    wireless-tools \
    net-tools

# Install Pwnagotchi
git clone https://github.com/evilsocket/pwnagotchi.git /usr/local/src/pwnagotchi
cd /usr/local/src/pwnagotchi
pip3 install -r requirements.txt
pip3 install .

# Create Pwnagotchi configuration directory
mkdir -p /etc/pwnagotchi
cat > /etc/pwnagotchi/config.toml << EOL
main.name = "ghost"
main.lang = "en"
main.whitelist = []
main.plugins.grid.enabled = true
main.plugins.grid.report = true
main.plugins.grid.exclude = []
main.plugins.auto-update.enabled = true
main.plugins.auto-update.install = true
main.plugins.auto-update.interval = 1
main.iface.mon = "mon0"
main.iface.mon_start_cmd = "airmon-ng start {interface} && iwconfig {interface} txpower 30"
main.iface.mon_stop_cmd = "airmon-ng stop {interface}"
main.iface.mon_max_blind_epochs = 50
main.iface.mon_hop_time = 0.5
EOL

# Enable and start Pwnagotchi service
cat > /etc/systemd/system/pwnagotchi.service << EOL
[Unit]
Description=Pwnagotchi AI
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/pwnagotchi
Restart=always
RestartSec=30

[Install]
WantedBy=multi-user.target
EOL

systemctl enable pwnagotchi.service

echo "Pwnagotchi installation complete!"
