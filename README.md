# IAC

[![build & test](https://github.com/Felix-Rm/iac/actions/workflows/cmake.yml/badge.svg)](https://github.com/Felix-Rm/iac/actions/workflows/cmake.yml)
[![Sonar Cube Static Analysis](https://sonarcloud.io/api/project_badges/measure?project=Felix-Rm_iac&metric=ncloc)](https://sonarcloud.io/dashboard?id=Felix-Rm_iac)
[![Sonar Cube Static Analysis](https://sonarcloud.io/api/project_badges/measure?project=Felix-Rm_iac&metric=alert_status)](https://sonarcloud.io/dashboard?id=Felix-Rm_iac)


## About

Iac is a communication library enabling sending data-packets between processes, computers and microcontrollers.
Packages can be sent over any connection type implementing basic read and writes.

The following connection types have builtin support:
- Internal Loopback (Communication between multiple nodes in one program)
- TCP-Sockets
- TCP-Sockets on NodeMCU microcontrollers