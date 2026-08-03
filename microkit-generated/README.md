# Microkit Generated Workspace

This directory holds the rendered Microkit-shaped source snapshot produced by
`scripts/render-microkit-generated.sh`.

## Layout

- `generated/manifest.json`
- `generated/README.md`
- `generated/<component>/component.h`
- `generated/<component>/component.json`
- `generated/<component>/main.c`

The files under `generated/` are intentionally shaped like SDK output so they
can be copied into a real seL4 / Microkit workspace or regenerated from the
host-side manifest when the service graph changes.

The rendered stubs use the Microkit callback surface (`init`, `notified`, and
`protected`) rather than the host-side `microkit_main` compatibility shim.

## Regeneration

```sh
./scripts/render-microkit-generated.sh
```

The bootstrap script also renders the same workspace structure into a chosen
seL4 workspace directory.
