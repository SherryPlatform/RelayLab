# RelayLab

The out-of-box Linux VM instance for robotics related educational scenarios
based on [NanaBox](https://github.com/M2Team/NanaBox).

Work In Progress

## Why named RelayLab?

In 2018, as a college student in the sophomore year, had selected a free
elective course called ROS Robotics Efficient Programming taught by
[Dr. Relay Zhang](https://blog.csdn.net/zhangrelay). I was very impressed
and benefited from this course because he will show the industrial robotics
scenarios at every course beginning, then start the live C++ and Python coding
demonstration in the Ubuntu environment with ROS Kinetic Kame installed. It's
too hardcore for most students because they are used to deal with the easy
courses, only the step of installing the Ubuntu environment with ROS Kinetic
Kame, they will use over a half semester. I think it's waste of time for
stucking at installing Linux distribution instead of touching the detail of
robotics with ROS. I had no idea to improve that to make teacher confident for
his course and his life, but he was very happy to meet me, and suggested me to
use Autoware to build the Autonomous Driving solution in projects from the IoT
and Connected Vehicle laboratory in the college.

In 2020, I had heard Hyper-V GPU-PV (paravirtualization) feature from
[DirectX ❤ Linux](https://devblogs.microsoft.com/directx/directx-heart-linux).
I hoped I can use the similar stack to achieve the goal of my college laboratory
teacher (not Dr. Relay Zhang) which is building Autonomous Driving solution for
educational scenarios, but I cannot achieve that due to meet something force
majeure. That's my regret.

In 2022, I started to build Hyper-V based virtualization software because VMware
Workstation was more buggy than I thought. I have choose Hyper-V Host Compute
System API because [Ben (Bingxing) Wang](https://github.com/imbushuo) told me
this API is more easier than Hyper-V WMI Providers. This project is finally
called [NanaBox](https://github.com/M2Team/NanaBox). I also wrote an e-mail to
[John Starks](https://github.com/jstarks) to diagnose some issues from Hyper-V
Host Compute System API and started with these words:

> I am trying to create an open-source third-party out-of-box-experience Hyper-V
  client based on Host Compute System recently for meeting my personal use and
  trying to create an autonomous driving framework for educational use based on
  it. (You may ask why I do it with Hyper-V HCS. Because GPU-PV and Linux Kernel
  Direct Boot are useful for me to achieve the goal.)

In 2025, [Windows Subsystem for Linux (WSL) 2](https://github.com/microsoft/WSL)
has been open source, and I have learned a lot from the implementation. With my
previous experience of using Hyper-V Host Compute System API, I think it's time
to build the out-of-box Linux VM instance for robotics related educational
scenarios based on [NanaBox](https://github.com/M2Team/NanaBox). I think it can
resolve my regret in 2020 and also can give some help for my ROS college course
teacher.

Because I think the project name of this idea should be started with R because
the robotics is started with R. I think maybe I can called this project RelayLab
because I hope this implementation can help Dr. Relay Zhang improve his course.

I also put this name to Large Language Model called DeepSeek, it has came out
many other ideas for this name. So, I think this project name is OK to use.

## Documents

- [License](License.md)
- [Release Notes](ReleaseNotes.md)
- [Versioning](Versioning.md)
