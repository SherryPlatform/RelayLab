# RelayLab.Kernel.Configurations

Configurations for Mainline Linux Kernel used in RelayLab.

## Version

Currently RelayLab uses Linux Kernel 6.12 series, and the Linux Kernel
configurations is based on these Debian packages:

- https://mirrors.tuna.tsinghua.edu.cn/alpine/v3.22/main/x86_64/linux-virt-6.12.40-r0.apk

## Flavors

### Hyper-V Generation 2 Virtual Machines

You can choose these configurations for Azure, Hyper-V and NanaBox.

Note: NanaBox is a third-party lightweight XAML-based out-of-box-experience 
oriented Hyper-V virtualization software based on Host Compute System API, 
Remote Desktop ActiveX control and XAML Islands, project repository available
at https://github.com/M2Team/NanaBox. NanaBox is the reference and prototype
virtualization platform for RelayLab.

- [x64, generic](config-x64-RelayLab.NanaBox)

## License

RelayLab.Kernel.Configurations is distributed under the same license from Linux
Kernel because these are the Linux Kernel configurations. For more information,
have a look at https://github.com/torvalds/linux/blob/master/COPYING.
