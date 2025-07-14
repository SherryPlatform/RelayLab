# RelayLab.Kernel.Configurations

Configurations for Mainline Linux Kernel used in RelayLab.

## Version

Currently RelayLab uses Linux Kernel 6.12 series, and the Linux Kernel
configurations is based on these Debian packages:

- https://mirrors.tuna.tsinghua.edu.cn/debian/pool/main/l/linux/linux-headers-6.12.37+deb13-cloud-amd64_6.12.37-1_amd64.deb
- https://mirrors.tuna.tsinghua.edu.cn/debian/pool/main/l/linux-signed-amd64/linux-image-6.12.37+deb13-cloud-amd64_6.12.37-1_amd64.deb

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
