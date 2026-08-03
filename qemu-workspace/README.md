# seL4 Workspace Scaffold

This directory is a local workspace stub for the HubOS seL4 integration.

The pinned manifest is `hubos.xml`, generated from
`seL4test-manifest/source-boundaries.json` and copied from
`seL4test-manifest/hubos.xml`.

Typical next steps:

1. initialize a repo workspace using the pinned manifest
2. sync the official seL4 dependencies
3. build the kernel and user-space projects with the upstream seL4 toolchain
4. map the root task, resource registry, capability manager, session manager,
   hub, driver service, and network server into separate user-space services
5. consult the generated source boundary summary before selecting optional
   backend assets
6. use the staged `upstream-mirror/` tree when you want to inspect the local
   clones that back the manifest

The manifest is intentionally pinned to public GitHub repositories from the
seL4 ecosystem plus any board-specific driver repositories listed in
`driver_projects`.
