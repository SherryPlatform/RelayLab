# RelayLab.RootFileSystem.Sample

The typical deployment root file system sample for RelayLab.

**Work In Progress**

## Install Xubuntu 24.04 Minimal

First, create a virtual machine with 32 GB of virtual hard disk first.

Then, install Xubuntu 24.04 Minimal for making lightweight root file system,
this example uses xubuntu-24.04.3-minimal-amd64.iso.

Here are some configuration this example used:

- Partition Table
  - ESP, /boot/efi, 100 MB
  - EXT4, /, remaining space
- Login Details
  - Name: Relay Lab
  - Computer Name: RelayLab
  - Username: relaylab
  - Password: rl

## Stage 0

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

### Install Ubuntu Azure Kernel

For smaller image size, we need to install and switch to the Ubuntu Azure Kernel
before removing the generic kernel.

> sudo apt install --no-install-recommends --no-install-suggests linux-azure

### Reboot virtual machine

Reboot the virtual machine for applying the newly installed kernel.

> sudo reboot
