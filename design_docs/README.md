# RdkEponManager - Design Documentation

**Version:** 1.0  
**Date:** December 2, 2025  
**Project:** RDK EPON Manager

## Overview

The RdkEponManager is an RDK (Reference Design Kit) application responsible for controlling and configuring EPON (Ethernet Passive Optical Network) lower layers. It serves as a middleware between the EPON HAL (Hardware Abstraction Layer) and other RDK components.

## Purpose

RdkEponManager provides a unified interface for EPON management in RDK-based systems, handling:
- EPON HAL initialization and event management
- TR-181 data model implementation via RBus/DBus
- Telemetry and statistics collection
- Interface with WanManager for link status updates

## Key Features

- **Event-driven architecture** for HAL event processing
- **Multi-threaded design** for concurrent operations
- **Statistics caching mechanism** (30-second TTL)
- **Periodic statistics harvesting** (every 15 minutes)
- **Comprehensive logging and telemetry integration**

## Documentation Structure

This design is organized into the following documents:

1. **[Requirements](01_Requirements.md)** - Functional and non-functional requirements
2. **[Architecture](02_Architecture.md)** - High-level system architecture and diagrams
3. **[Component Design](03_Component_Design.md)** - Detailed component specifications
4. **[Sequence Diagrams](04_Sequence_Diagrams.md)** - Event flows and interactions
5. **[Thread Architecture](05_Thread_Architecture.md)** - Multi-threading design
6. **[Data Flow](06_Data_Flow.md)** - Data flow through the system
7. **[Configuration](07_Configuration.md)** - Configuration and deployment

## Quick Start

### System Components

- **EPON Controller** - Main coordinator and initializer
- **RBus/DBus Thread** - TR-181 DML handler
- **HAL Event Listener Thread** - Processes HAL events
- **Stats Polling Thread** - Periodic statistics harvester
- **Logger Module** - RDK logger integration
- **Telemetry Module** - T2 integration

### Key Technologies

- **Communication Bus**: RBus/DBus
- **Data Model**: TR-181
- **Telemetry**: T2
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

**Document Status:** Draft  
**Last Updated:** December 2, 2025
