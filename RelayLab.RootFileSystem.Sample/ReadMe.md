# RelayLab.RootFileSystem.Sample

The typical deployment root file system sample for RelayLab.

## Install Xubuntu 24.04 Minimal

First, create a virtual machine with 20 GB of virtual hard disk first.

Then, install Xubuntu 24.04 Minimal for making lightweight root file system,
this example uses xubuntu-24.04.3-minimal-amd64.iso.

Here are some configuration this example used:

- Partition Table
  - ESP, /boot/efi, 104 MB (will be 100 MiB)
  - EXT4, /, remaining space
- Login Details
  - Name: Relay Lab
  - Computer Name: RelayLab
  - Username: relaylab
  - Password: rl

## Stage 0 Configuration

### Open local terminal

Press "Ctrl+Alt+T" or find "Terminal Emulator" in the launcher will open the
local terminal in the target virtual machine.

### Modify APT source

Change the APT source will helpful to install and update packages.

- Remove default APT source configuration backup to make sure modification
  properly.
  > sudo rm /etc/apt/sources.list.d/ubuntu.sources.curtin.orig
- Edit APT source with GNU nano.
  > sudo nano /etc/apt/sources.list.d/ubuntu.sources
- Refresh the APT source.
  > sudo apt update

### Install OpenSSH Server

Enable SSH server will helpful for the following configurations.

```
sudo apt install --no-install-recommends --no-install-suggests openssh-server
sudo systemctl enable ssh
sudo systemctl start ssh
```

### Exit local terminal

Before switch to the SSH connection, exit the local terminal.

> exit

### Switch to SSH connection

Open Windows Terminal in the host OS, then connect the target virtual machine
with SSH. Here is the example.

> ssh relaylab@RelayLab

### Remove snap support

For making a lightweight Ubuntu series image, it's necessary to remove snap.
Fortunately, Xubuntu 24.04 Minimal won't install any snap package, we only need
to remove snapd.

```
sudo apt purge snapd
sudo apt autopurge
cat <<EOF | sudo tee /etc/apt/preferences.d/nosnap.pref
# To prevent repository packages from triggering the installation of Snap,
# this file forbids snapd from being installed by APT.
# For more information: https://linuxmint-user-guide.readthedocs.io/en/latest/snap.html
Package: snapd
Pin: release a=*
Pin-Priority: -10
EOF
```

### Install Firefox

It's necessary to have a web browser. We need to install Firefox from Mozilla
APT source because Ubuntu series only provide snap version of Firefox.

Here are some references for using the Mozilla APT source.

- https://support.mozilla.org/en-US/kb/install-firefox-linux#w_install-firefox-deb-package-for-debian-based-distributions
- https://mirrors.tuna.tsinghua.edu.cn/help/mozilla

```
sudo install -d -m 0755 /etc/apt/keyrings
wget -q https://packages.mozilla.org/apt/repo-signing-key.gpg -O- | sudo tee /etc/apt/keyrings/packages.mozilla.org.asc > /dev/null
gpg -n -q --import --import-options import-show /etc/apt/keyrings/packages.mozilla.org.asc | awk '/pub/{getline; gsub(/^ +| +$/,""); if($0 == "35BAA0B33E9EB396F59CA838C0BA5CE6DC6315A3") print "\nThe key fingerprint matches ("$0").\n"; else print "\nVerification failed: the fingerprint ("$0") does not match the expected one.\n"}'
echo "deb [signed-by=/etc/apt/keyrings/packages.mozilla.org.asc] https://mirrors.tuna.tsinghua.edu.cn/mozilla/apt mozilla main" | sudo tee -a /etc/apt/sources.list.d/mozilla.list > /dev/null
echo '
Package: *
Pin: release a=mozilla
Pin-Priority: 1000
' | sudo tee /etc/apt/preferences.d/mozilla
sudo apt update
sudo apt install --no-install-recommends --no-install-suggests firefox
```
### Install essential tools

> sudo apt install --no-install-recommends --no-install-suggests htop git neofetch build-essential bleachbit curl gnupg2

### Stop Desktop Environment

> sudo init 3

### Fix X.Org issue when GPU-PV is enabled

```
echo '
Section "Device"
    Identifier "Display 0"
    Driver "fbdev"
EndSection
' | sudo tee /etc/X11/xorg.conf.d/10-vmbus-synthvid.conf
```

## Remove unnecessary X.Org drivers

```
sudo apt purge xserver-xorg-video-all
sudo apt autopurge
sudo apt install --no-install-recommends --no-install-suggests xserver-xorg-video-fbdev
```

### Start Desktop Environment

> sudo init 5

### Install ROS2

```
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key  -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] https://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu noble main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
sudo apt update
sudo apt install --no-install-recommends --no-install-suggests ros-dev-tools ros-jazzy-desktop
```

## Install Webots

```
sudo curl -sSL https://cyberbotics.com/Cyberbotics.asc -o /etc/apt/keyrings/Cyberbotics.asc
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/Cyberbotics.asc] https://cyberbotics.com/debian binary-amd64/" | sudo tee /etc/apt/sources.list.d/Cyberbotics.list > /dev/null
sudo apt update
sudo apt install --no-install-recommends --no-install-suggests webots ros-jazzy-webots-ros2
```

### Update packages

> sudo apt upgrade

### Install Ubuntu Azure Kernel

For smaller image size, we need to install and switch to the Ubuntu Azure Kernel
before removing the generic kernel.

> sudo apt install --no-install-recommends --no-install-suggests linux-azure

### Remove generic kernel

```
sudo apt purge linux-firmware linux-*-generic
sudo apt autopurge
```

### Cleanup caches

Use BleachBit as root and standard user to do that in your virtual machine
window. After that, switch to your SSH window.

### Zero the free space

It's necessary for shrink the virtual hard disk.

```
dd if=/dev/zero of=zeroedspace bs=1M
sync
rm zeroedspace
```

### Poweroff virtual machine

> sudo poweroff

### Shrink virtual hard disk

You can use NanaBox or PowerShell to achieve this goal.

## Work In Progress Configuration Snippets

### Prepare RelayLab Workspace

> git clone https://github.com/SherryPlatform/RelayLab

### Compile Custom Kernel

```
sudo apt update
sudo apt install --no-install-recommends --no-install-suggests gnupg2 build-essential flex bison dwarves libssl-dev libelf-dev cpio ncurses-dev
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.41.tar.xz
unxz -v linux-6.12.41.tar.xz
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.41.tar.sign
gpg2 --locate-keys torvalds@kernel.org gregkh@kernel.org
gpg2 --verify linux-6.12.41.tar.sign
tar xvf linux-6.12.41.tar
rm linux-6.12.41.tar
rm linux-6.12.41.tar.sign
mkdir linux-stable-output
cd linux-6.12.41
make KCONFIG_CONFIG=/home/relaylab/RelayLab/RelayLab.Kernel.Configurations/config-x64-RelayLab.NanaBox O=/home/relaylab/linux-stable-output -j4
sudo make KCONFIG_CONFIG=/home/relaylab/RelayLab/RelayLab.Kernel.Configurations/config-x64-RelayLab.NanaBox O=/home/relaylab/linux-stable-output INSTALL_MOD_STRIP=1 modules_install
sudo make KCONFIG_CONFIG=/home/relaylab/RelayLab/RelayLab.Kernel.Configurations/config-x64-RelayLab.NanaBox O=/home/relaylab/linux-stable-output install
sudo /home/relaylab/RelayLab/RelayLab.Kernel.Configurations/build-headers.sh --source "/home/relaylab/linux-6.12.41" --config "/home/relaylab/RelayLab/RelayLab.Kernel.Configurations/config-x64-RelayLab.NanaBox" --arch x86 --build-output "/home/relaylab/linux-stable-output" --output "/usr/src/linux-6.12.41-RelayLab.Kernel.NanaBox"
sudo chgrp -R root /usr/src/linux-6.12.41-RelayLab.Kernel.NanaBox
sudo chown -R root /usr/src/linux-6.12.41-RelayLab.Kernel.NanaBox
sudo rm /usr/lib/modules/6.12.41-RelayLab.Kernel.NanaBox/build
sudo ln -s /usr/src/linux-6.12.41-RelayLab.Kernel.NanaBox /usr/lib/modules/6.12.41-RelayLab.Kernel.
NanaBox/build
cd ~
rm -rf linux-stable-output
rm -rf linux-6.12.41
```

## Remove Ubuntu Azure Kernel

```
sudo apt purge linux-*azure*
sudo apt autopurge
```

### Disable swap temporarily

For smaller image size, we need to disable swap temporarily.

- Disable swap.img
  > sudo swapoff /swap.img
- Edit fstab to comment out swap.img
  > sudo nano /etc/fstab
- Remove swap.img
  > sudo rm /swap.img

### Compile Custom Mesa3D

**This section is not verified currently**

```
git clone -b 24.3 --depth 1 https://gitlab.freedesktop.org/mesa/mesa.git
sudo apt update
sudo apt install meson glslang-tools libclc-19-dev pkgconf python3-mako libdrm-dev llvm llvm-spirv-18 libllvmspirvlib-18-dev libclang-dev libwayland-dev libwayland-egl-backend-dev libx11-xcb-dev libxcb-randr0-dev libxext-dev libxfixes-dev libxcb-glx0-dev libxcb-shm0-dev libxcb-dri3-dev libxcb-present-dev libxshmfence-dev libxxf86vm-dev libxrandr-dev libglvnd-dev bison flex
cd mesa
meson setup --strip -D gallium-drivers=swrast,d3d12 -D vulkan-drivers=swrast,microsoft-experimental -D microsoft-clc=enabled -D video-codecs=all -D osmesa=true -D glvnd=enabled build/
ninja -C build/
sudo ninja -C build/ install
sudo ldconfig
sudo apt install mesa-utils
rm -rf mesa
```
