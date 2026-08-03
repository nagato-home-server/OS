<!--
     Copyright 2018, Data61, CSIRO

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

sel4test-manifest
=================

The `sel4test` project aims to test the seL4 kernel and some of its user libraries on many different
targets.

For instructions on using this repository, see the [seL4Test
page](https://docs.sel4.systems/seL4Test) on the [seL4 docsite](https://docs.sel4.systems).

See [Host Dependencies](https://docs.sel4.systems/HostDependencies) for required toolchains and
dependencies to build `sel4test`.

For this repository, see [`hubos.xml`](./hubos.xml) for a pinned official GitHub dependency
manifest that matches the HubOS integration target.

For a local workspace scaffold, use [`scripts/bootstrap-sel4-workspace.sh`](../scripts/bootstrap-sel4-workspace.sh).
