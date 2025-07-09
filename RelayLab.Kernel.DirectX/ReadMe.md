# RelayLab.Kernel.DirectX

Out-of-tree Linux Kernel Module for dxgkrnl from Windows Subsystem for Linux 2
(WSL 2) but used in RelayLab.

## Usage

```
make KCPPFLAGS="-DDEBUG"
sudo make load
sudo make unload
```

## Version

The dxgkrnl kernel module in this project current use commit 
https://github.com/microsoft/WSL2-Linux-Kernel/tree/d4ec8a2216634e7c0ca058214febce3a2f029bc6.

Also, the Linux Kernel branches from https://github.com/staralt/dxgkrnl-dkms and
https://github.com/Nevuly/WSL2-Linux-Kernel-Rolling-LTS are also referenced by
this module.

## License

RelayLab.Kernel.DirectX is distributed under the same license from Linux Kernel
because this is a out-of-tree Linux kernel module. For more information, have a
look at [COPYING](COPYING).
