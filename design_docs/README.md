# RdkEponManager — Design Documentation

**Version:** 1.1  
**Date:** May 15, 2026  
**Project:** RDK EPON Manager

## Overview

The RdkEponManager is an RDK (Reference Design Kit) application responsible for
controlling and configuring EPON (Ethernet Passive Optical Network) lower layers.
It serves as middleware between the EPON HAL (Hardware Abstraction Layer) and
other RDK components.

## Purpose

RdkEponManager provides a unified interface for EPON management in RDK-based
systems, handling:
- EPON HAL initialization and event management
- TR-181 data model implementation via RBus/DBus
- Telemetry and statistics collection
- Interface with WanManager for link status updates

## Key Features

- **Event-driven architecture** for HAL event processing
- **Multi-threaded design** for concurrent operations
- **Statistics caching mechanism** (30-second TTL)
- **Comprehensive logging and telemetry integration**

## Documentation Structure

| # | Document | Description |
|---|----------|-------------|
| 01 | [Requirements](01_Requirements.md) | Functional and non-functional requirements |
| 02 | [Architecture](02_Architecture.md) | High-level system architecture and diagrams |
| 03 | [Component Design](03_Component_Design.md) | Detailed component specifications |
| 04 | [Sequence Diagrams](04_Sequence_Diagrams.md) | Event flows and interactions |
| 05 | [Thread Architecture](05_Thread_Architecture.md) | Multi-threading design |
| 06 | [Configuration](06_Configuration.md) | Configuration and deployment |
| 07 | [Telemetry Design](07_Telemetry_Design.md) | Telemetry module architecture, API, and T2 integration |
| ~~08~~ | *(merged into 07)* | Module design merged into doc 07 (v1.2) |
| 09 | [TR-181 & Telemetry Reference](09_TR181_Telemetry_Reference.md) | TR-181 parameters and telemetry event spec — [HTML](09_TR181_Telemetry_Reference.html) |
| 10 | [Harvester Future Direction](10_Harvester_Future_Direction.md) | Deferred: design guide for future Avro periodic report |
| — | [Telemetry Acceptance Criteria](Telemetry_Acceptance_Criteria.md) | MoSCoW acceptance criteria for the telemetry story |
| — | [EPON HAL Proposal](EPON_HAL_Proposal.md) | HAL interface proposal |

## Quick Start

### System Components

- **EPON Controller** — Main coordinator and initializer
- **RBus/DBus Thread** — TR-181 DML handler
- **HAL Event Listener Thread** — Processes HAL events
- **Stats Polling Thread** — Periodic statistics collection
- **Logger Module** — RDK logger integration
- **Telemetry Module** — T2 integration (34 event markers)

### Key Technologies

- **Communication Bus**: RBus/DBus
- **Data Model**: TR-181
- **Telemetry**: T2 (`t2_event_s` / `t2_event_d`)
- **Hardware Interface**: EPON HAL

## Glossary

| Term | Description |
|------|-------------|
| EPON | Ethernet Passive Optical Network |
| ONU | Optical Network Unit |
| OLT | Optical Line Terminal |
| LLID | Logical Link ID |
| HAL | Hardware Abstraction Layer |
| TR-181 | Technical Report 181 (Data Model) |
| RBus | RDK Bus (IPC mechanism) |
| DML | Data Model Library |
| T2 | Telemetry 2.0 |
| TTL | Time To Live |

## References

- TR-181 Device Data Model Specification
- RDK Documentation (rdkcentral.com)
- IEEE 802.3ah EPON Standard
- RBus API Documentation
- RDK Logger API Guide

---

**Document Status:** Active  
**Last Updated:** May 15, 2026
