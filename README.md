# Pyrrha firmware

This repository contains the [Pyrrha](https://github.com/Pyrrha-Platform/Pyrrha) solution firmware for the [Pyrrha hardware](https://github.com/Pyrrha-Platform/Pyrrha-Hardware).

The firmware has undergone several iterations based on field testing of its functionality in controlled burn environments in Spain and in the labs at universities and private companies.

It is now ready for a redesign to ensure it can be reliably connected with better components to withstand additional rigorous international conditions.

[![License](https://img.shields.io/badge/License-Apache2-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0) [![Slack](https://img.shields.io/static/v1?label=Slack&message=%23prometeo-pyrrha&color=blue)](https://callforcode.org/slack)

## Recreating the prototype

- Install the Arduino IDE and connect it to a [Prometeo hardware device](https://github.com/Pyrrha-Platform/Pyrrha-Hardware).
- Load the latest firmware sketch on the device.

## Design review next steps

There are several upgrades that are planned for the next version of the fireware to ensure it can be reliably embedded and to ensure it meets international standards for reliability.

The goal that we hope to achieve with the community is a new iteration that takes into consideration portability, stability, and best practices for real-time systems.

Each of these firmware recommendations are documented as [issues](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues) in this repository, or tracked in the [Hardware repo](https://github.com/Pyrrha-Platform/Pyrrha-Hardware).

- [Toolchains](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/31)
- [Real Time Operating System](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/30)
- [Hardware abstraction](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/29)
- [Field device management](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/28)
- [Design for test](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/27)
- [Style guide](https://github.com/Pyrrha-Platform/Pyrrha-Firmware/issues/32)

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting Pyrrha pull requests.

## License

This project is licensed under the Apache 2 License - see the [LICENSE](LICENSE) file for details.
