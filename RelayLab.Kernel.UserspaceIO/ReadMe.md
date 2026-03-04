# RelayLab.Kernel.UserspaceIO

Out-of-tree Linux Kernel Module for uio and uio_hv_generic from Linux Kernel may
not be provided by Linux distros but used in RelayLab.

## Usage

```
make KCPPFLAGS="-DDEBUG"
sudo make load
sudo make unload
```

## Version

The uio and uio_hv_generic kernel modules in this project current use Linux
Kernel 6.18.16 implementation.

## License

RelayLab.Kernel.DirectX is distributed under the same license from Linux Kernel
because this is a out-of-tree Linux kernel module. For more information, have a
look at [COPYING](COPYING).
