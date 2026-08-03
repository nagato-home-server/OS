# Hub-Oriented Session OS Specification

Version 1.1 Draft

## 0. Terminology

### Resource

Resource is any OS-managed entity.

Examples:

- PCI Function
- USB Device
- Network Interface
- Block Device
- GPU Context

### Capability

A Capability is authority over a Resource.

Capability is the only basis for authorization.

### Session

A Session is a contract for Resource usage.

A Session holds a set of Capabilities and usage state.

### Session Owner

A Session Owner is an entity that manages Sessions.

Examples:

- App
- Container Server
- VM Server
- NIC Server
- GPU Server

### Server

A Server owns Resources and provides Sessions.

### App VM Runtime

An App VM Runtime is a managed guest execution profile selected by HubOS for an
App VM.

Examples:

- full Linux guest
- small BSD-style runtime guest
- unikernel-style appliance runtime

### Hub

The Hub is the control entity that performs:

- Session creation
- Authorization
- Name resolution
- Lifecycle coordination

Implementation split for the current codebase:

- Hub: name resolution and service discovery
- Capability Manager: authorization and capability verification
- Session Manager: session tree management, session brokering, and lifecycle coordination
- Root Task: bootstrap-only publication of service endpoints

## 1. Purpose

This OS uses seL4 as a minimal kernel and builds a Capability- and Session-centered Hub-oriented operating system on top of it.

The goals are:

- Strong isolation
- High fault tolerance
- Driver restartability
- Unified container and VM management
- User-selectable App VM runtime installation and selection
- High-performance I/O
- Dynamic device reconfiguration
- Capability-based security

## 2. Design Principles

### Principle 1: Everything is a Session Owner

All resource-managing entities are handled under one model.

The following are all Session Owners:

- App
- Container
- VM
- NIC Server
- GPU Server
- NVMe Server
- USB Server

### Principle 2: Separation of Control Plane and Data Plane

Control Plane:

- Hub
- Policy
- Routing
- Session Broker

Data Plane:

- Device Server
- DMA
- Shared Ring

The Hub does not forward data.

### Principle 3: Capability Only

Authorization is performed only through Capabilities.

Forbidden:

- Session ID authentication
- Name authentication
- Password authentication

### Principle 4: Device Ownership

Every device must have an owner.

PCI Device
-> Device Server
-> Session
-> App

An App does not directly own a device.

## 3. System Structure

Hardware
-> seL4 Kernel
-> Root Task
  - Resource Registry
  - Capability Manager
  - Session Manager
  - Memory Manager
  - DMA Manager
  - Hub
  - Driver Registry
  - Driver Loader
  - Bus Managers
  - Device Servers
  - System Servers
  - App Managers
  - Supervisor

## 3.5 Trust Hierarchy

### Tier 0

Fully trusted:

- seL4 Kernel

### Tier 1

System trust base:

- Root Task
- Resource Registry
- DMA Manager

### Tier 2

Authority management:

- Capability Manager
- Session Manager

### Tier 3

Hardware management:

- Bus Managers
- Driver Registry
- Driver Loader

### Tier 4

Service layer:

- Device Servers
- Network Server
- Storage Server
- Display Server

### Tier 5

Consumer layer:

- Container
- VM
- Application

## 4. seL4 Responsibilities

seL4 is responsible only for:

- Capability
- IPC
- Address Space
- Scheduling
- Notification
- IRQ
- Untyped Memory
- Page Mapping

seL4 does not provide:

- Network Stack
- File System
- Driver Policy
- Device Naming
- Session Policy
- Container Policy

## 5. Resource Registry

The Resource Registry is the system's unique resource ledger.

Responsibilities:

- Resource ID issuance
- Resource state management
- Resource revocation management
- Audit logging

### Resource ID

Resource IDs must be:

- Unique
- Non-reusable
- Auditable

Examples:

- `resource://pci/0000:01:00.0`
- `resource://usb/1-2`
- `resource://network/nic0`
- `resource://storage/nvme0`

## 5.5 Resource Lifecycle

### Resource Creation

The following may create Resources:

- PCIe Manager
- USB Manager
- I2C Manager
- SPI Manager
- Resource Registry

The final authority for Resource creation and Resource ID issuance is the Resource Registry.

Bus Managers may discover, classify, and request registration, but they do not have final ledger authority.

### Resource States

- DISCOVERED
- CLASSIFIED
- BOUND
- READY
- FAILED
- QUARANTINED
- RETIRED

## 6. Capability Manager

Responsibilities:

- copy
- mint
- revoke
- transfer
- inspect

Forbidden:

- Device creation
- Service creation
- Session creation
- Resource registration

## 7. Session

Session is the basic unit of the OS.

```text
Session {
    SessionID
    Owner
    Parent
    Children
    CapabilitySet
    ResourceSet
    Lease
    Policy
    State
}
```

### Session Types

- Permanent
- Persistent
- Ephemeral
- Transactional

Sessions form a tree.

Example:

Container Session
-> Network Session
-> Storage Session
-> GPU Session

On parent deletion:

- Cascade Revoke is mandatory

## 7.5 Session Inheritance Rules

### Inheritable

The following may be inherited by child Sessions:

- Quota
- Namespace View
- Policy Context
- Session Metadata

### Non-inheritable

Automatic inheritance is forbidden for:

- MMIO Capability
- IRQ Capability
- DMA Capability
- Root Capability

### Delegation

Capabilities have attributes:

- `delegatable`
- `non-delegatable`

Parent Sessions may redelegate only `delegatable` Capabilities.

### Shared Resource

Shared Resources are reference-counted.

Examples:

- Network Namespace
- Shared Memory Pool
- Storage Volume

During Cascade Revoke:

- A Resource with `refcount > 0` must not be destroyed immediately

## 8. Hub

Responsibilities:

- Name Resolution
- Policy Enforcement
- Session Brokering
- Permission Validation
- Lifecycle Coordination

Forbidden:

- Packet Routing
- DMA
- Block Transfer
- GPU Command Execution

The Hub resolves names, but it does not authorize access by name.

Authorization is done only by Capability verification.

In the current implementation, the broader conceptual Hub is intentionally
split across dedicated services so that steady-state authorization remains in
the Capability Manager and session lifecycle management remains in the Session
Manager.

## 9. Memory Manager

Responsibilities:

- Untyped management
- Frame creation
- Frame reclamation
- VSpace management
- Shared Memory
- Huge Pages
- NUMA

## 10. DMA Manager

The DMA Manager is the only entity allowed to manage DMA.

Responsibilities:

- DMA Buffer Allocation
- IOMMU Mapping
- DMA Revoke
- DMA Audit

### Absolute Rule

No DMA outside explicitly mapped regions.

### DMA Mapping State

- UNMAPPED
- MAPPING
- ACTIVE
- QUIESCING
- REVOKED

### DMA Revoke Conditions

DMA may transition to `REVOKED` only after:

- device queue empty
- outstanding DMA complete
- interrupts drained

## 11. Bus Managers

Responsibilities:

- Device Discovery
- Enumeration
- Capability Preparation

Ownership is forbidden.

Targets:

- PCIe Manager
- USB Manager
- I2C Manager
- SPI Manager

## 12. Driver System

### Driver Registry

Driver ledger.

Managed items:

- Vendor ID
- Device ID
- Class Code
- Driver Package

### Driver Loader

Responsibilities:

- Spawn
- Stop
- Restart
- Rebind
- Sandbox Launch
- Driver Package handling

### Driver Package

Required contents:

- Manifest
- Binary
- Signature
- Hash
- Dependencies

Signature is mandatory.

## 12.5 Driver Trust Chain

### Root Key

Firmware provides the Root Public Key.

### Driver Package Requirements

Mandatory elements:

- Manifest
- Binary
- Signature
- Hash
- Version

### Verification

Driver Loader verifies:

- Signature
- Hash
- Dependencies
- Compatibility

### Verification Failure

Failure state:

- QUARANTINED

Startup is forbidden.

### Rollback Protection

Optional:

- Minimum Driver Version

## 13. Device Lifecycle

### Startup

- DISCOVERED
- CLASSIFIED
- BOUND
- READY

### Error

- FAILED
- QUARANTINED

### Reassignment

- DRAINING
- REBINDING
- READY

## 14. Device Management Models

### per_device

- NIC -> NIC Server
- NVMe -> NVMe Server
- GPU -> GPU Server

### grouped

- USB -> USB Server

### hybrid

- USB Server
- HID
- Audio
- Storage Bridge
- NIC Bridge

### passthrough

- SR-IOV VF
- Dedicated Queue
- Virtual Function

Direct assignment is allowed only in this mode.

## 15. Device Servers

Examples:

- NIC Server
- NVMe Server
- GPU Server
- USB Server
- Audio Server

Only Device Servers may own devices.

Responsibilities:

- MMIO
- IRQ
- Reset
- Device Control

## 16. Network Architecture

Structure:

App
-> Network Session
-> NIC Session
-> NIC Server
-> NIC

Network Server responsibilities:

- Routing
- Address Management
- Namespace
- Firewall
- Policy

Packet relaying is forbidden.

Multiple NICs are supported:

- eth0
- eth1
- wifi0
- vpn0

Selection is governed by Policy.

## 16.5 Name Resolution and Authorization Separation

### Principle

Names are never authorization tokens.

Names are identification only.

### Examples

- `network.nic0`
- `storage.nvme0`
- `display.gpu0`

These do not imply authority.

### Forbidden

`connect("network.nic0")` alone must not grant access.

### Correct Flow

- `resolve("network.nic0")`
- obtain Service Descriptor
- obtain Capability
- connect using Capability

### Hub Rule

The Hub performs Name Resolution.

Authorization is performed only by Capability verification.

## 17. Storage Architecture

Structure:

App
-> VFS
-> Storage Server
-> NVMe Session
-> NVMe Server

High-performance usage:

App
<-> Block Session
<-> NVMe Server

## 18. USB Architecture

USB Server:

- HID
- Storage Bridge
- Audio Bridge
- Camera Bridge
- NIC Bridge

## 19. App Model

### Small App

Normal application.

### Service App

Persistent service.

### Container App

Manages multiple processes.

### VM App

Guest OS.

### Large App

Consumes substantial resources.

Examples:

- Linux VM
- Browser
- Database
- AI Runtime

## 20. Container Architecture

Hub
-> Container Server
-> Container Apps

Container Server responsibilities:

- Process Lifecycle
- Filesystem View
- Capability Delegation
- Resource Quota
- Namespace Handle

### Revised Network Structure

Container
-> Namespace Handle
-> Network Server
-> Routing
-> Firewall
-> Address

Network Namespace ownership belongs to the Network Server.

The Container holds only a Network Namespace Handle.

## 21. VM Architecture

Example: Linux VM

Structure:

VM Server
|- Guest Memory
|- vCPU
|- virtio-net Session
|- virtio-blk Session
`- vGPU Session

## 22. Resource Envelope

For Large Apps only.

```text
ResourceEnvelope {
    Memory
    HugePages
    CPU Cores
    Scheduling Budget
    NUMA Node
    IO Sessions
    Device Sessions
}
```

## 23. Driver Rebinding

Modes:

- force
- graceful
- sandbox

### Graceful

READY
-> DRAINING
-> reject new Sessions
-> wait for in-flight I/O completion
-> MMIO revoke
-> IRQ revoke
-> DMA revoke
-> UNBOUND
-> REBINDING
-> new Driver
-> READY

### Force

READY
-> MMIO revoke
-> IRQ revoke
-> DMA revoke
-> force Session termination
-> REBINDING
-> new Driver
-> READY

### Sandbox

READY
-> start Sandbox Driver
-> verification
-> success
-> Graceful Rebind

### Required Rules

- MMIO revoke is mandatory
- IRQ revoke is mandatory
- DMA revoke is mandatory
- Simultaneous access by old and new Drivers is forbidden

## 24. Security Model

### Trust Boundaries

- seL4
- Resource Registry
- DMA Manager

### Important Rules

- Capability Manager does not create Resources
- Only Device Servers publish services
- Only DMA Manager permits DMA
- The Hub does not transfer data
- Only Session Capabilities authorize access

## 25. Audit

The following must be recorded:

- Capability Mint
- Capability Transfer
- Capability Revoke
- Driver Bind
- Driver Rebind
- Driver Unbind
- DMA Mapping
- DMA Unmapping
- Session Create
- Session Destroy

## 26. Boot Sequence

Firmware
-> seL4
-> Root Task
-> Resource Registry
-> Capability Manager
-> Memory Manager
-> DMA Manager
-> Hub
-> Driver Registry
-> Bus Managers
-> Device Discovery
-> Driver Binding
-> System Servers
-> App Managers
-> Apps

Parallel startup is allowed.

## 27. Final Definition

This OS is:

- Capability-Driven
- Session-Oriented
- Hub-Coordinated
- Device-Ownership-Based
- Microkernel Operating System

Control is performed by the Hub, Resources are managed as Sessions, devices are owned by dedicated Servers, and data flows through Direct Sessions. seL4 provides only minimal isolation, IPC, and Capability primitives.

For the current implementation, the Hub role is split so that the Hub handles
name resolution, the Capability Manager handles authorization, and the Session
Manager handles session tree and lifecycle coordination.

## 28. Consolidated Open Issues

The following items were ambiguous in v1.0 and are pinned down by the v1.1 draft below:

- Resource registration authority when multiple discovery managers exist.
- Trust boundaries and circular dependencies in the bootstrap chain.
- `Namespace View` and `Policy Context` inheritance semantics.
- Shared resource finalization beyond reference counting.
- Driver key rotation and revocation.
- Sandbox rebind verification.
- DMA failure states for non-responsive hardware.
- Explicit storage and display namespace ownership.
- Non-authoritative Service Descriptor semantics.
- Initial boot trust anchor and capability set.

## 29. Version 1.1 Draft

### 29.1 Resource Registration Authority

Discovery managers may detect, classify, and prepare resources, but they do not issue canonical Resource IDs.

The Resource Registry is the only authority that:

- assigns the final Resource ID
- records the authoritative resource entry
- transitions a resource from provisional to registered state
- records revocation and retirement

If multiple discovery managers report the same physical device, the Resource Registry merges them into one canonical record and rejects duplicate ownership claims.

### 29.2 Bootstrap Trust Chain

The bootstrap chain must be acyclic.

Allowed bootstrap dependencies:

- seL4 Kernel
- Root Task
- Resource Registry
- Capability Manager
- Memory Manager
- DMA Manager
- Hub

Prohibited bootstrap dependencies:

- any service that depends on a device driver to start
- any manager that requires DMA access before the DMA Manager is initialized
- any policy component that can only be validated by a later consumer-layer service

The Root Task receives only the minimal bootstrap capabilities needed to start the system trust base.

### 29.3 Session Inheritance

`Namespace View` and `Policy Context` are inherited as versioned immutable snapshots.

Rules:

- Child Sessions inherit a reference to the snapshot that existed at Session creation time.
- Parent updates do not mutate existing children.
- A Session may refresh to a newer snapshot only through an explicit policy action.
- Quota and Session Metadata remain inheritable under the existing rules.

### 29.4 Shared Resource Finalization

Shared Resources use reference counting for liveness, but destruction is always two-phase.

Phase 1:

- detach the resource from the revoking Session
- decrement `refcount`
- mark the resource as pending finalization if `refcount == 0`

Phase 2:

- the owning server or registry finalizer drains in-flight work
- the resource transitions to retired only after finalization completes

This applies to Network Namespace, Shared Memory Pool, Storage Volume, and other shared resources added later.

### 29.5 Driver Trust Chain

The Root Public Key is provided by firmware and anchors the initial driver trust set.

Driver trust rules:

- Driver Packages must be signed.
- The Driver Loader verifies signature, hash, dependencies, and compatibility.
- Key rotation is performed by a signed keyring update.
- Revocation is performed by a signed revocation record or an equivalent policy artifact.
- A revoked key may not authorize new driver startup.

If a driver fails trust validation, the device enters `QUARANTINED` and startup is forbidden.

### 29.6 Sandbox Rebind

Sandbox Rebind is a pre-rebind validation step.

Flow:

- start a sandboxed driver instance
- attach it to a shadow control path or emulated view
- run a bounded verification sequence
- compare the observed device-state transition against the expected profile
- only then allow Graceful Rebind

Failure rules:

- if validation fails before device side effects, the old driver remains active
- if side effects cannot be ruled out, the device enters `QUARANTINED`
- simultaneous access by old and new drivers remains forbidden

### 29.7 DMA Failure States

The DMA lifecycle gains one additional terminal failure state:

- `ABORTED`

`ABORTED` means the DMA Manager could not complete revoke or quiescence because the hardware stopped responding or an IOMMU operation could not be completed safely.

Rules:

- `REVOKED` is used only for successful completion.
- `ABORTED` is used for safe failure.
- A device in `ABORTED` must not be allowed to resume DMA until a recovery policy explicitly resets or retires it.

### 29.8 Storage and Display Namespaces

Storage and Display follow the same ownership split already defined for Network.

Ownership rules:

- Storage Namespace ownership belongs to the Storage Server.
- Display Namespace ownership belongs to the Display Server.
- Containers and Apps may hold only handles or capabilities to those namespaces.
- Names remain identification only and never grant authority.

### 29.9 Service Descriptors

Service Descriptor data is discovery metadata only.

Rules:

- a Service Descriptor may describe endpoints, versions, and policy hints
- a Service Descriptor may not authorize access by itself
- capability verification is the only authorization step
- a name lookup may return a descriptor, but the descriptor must still be paired with a valid Capability before use

### 29.10 Initial Boot Capability Set

The initial boot trust anchor is firmware plus the Root Public Key.

The initial capability set must be minimal and explicit:

- the Root Task may receive only the capabilities required to start the trust base
- device ownership capabilities are not part of the initial set
- discovery managers receive only the capabilities needed to register discovered resources
- any later service capability is minted only after registry and policy validation

This makes the boot sequence executable policy instead of an informal trust assumption.
