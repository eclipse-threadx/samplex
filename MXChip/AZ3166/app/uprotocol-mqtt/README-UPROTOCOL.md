# Build and deploy the AZ3166 MQTT demos

SampleX has two independent AZ3166 applications: `app/mqtt` is the original
upstream MQTT 3.1.1 demo, and `app/uprotocol-mqtt` publishes and receives
uProtocol 1.6 messages over MQTT 5. The latter uses the AZ3166 NetX Duo
submodule pinned to fork commit `0edae5f` and the uProtocol specification at
up-spec commit `3130bc5bfe661f60509b9d5e1d03995d608a18b2`.

Run these commands from the SampleX repository root. Initialize the board
submodules before building:

```sh
git submodule update --init MXChip/AZ3166/deps/lib/threadx MXChip/AZ3166/deps/lib/netxduo
```

## Build

Use the existing build script. Its `rebuild` argument clears the shared
`MXChip/AZ3166/build` directory, which is necessary when switching app
configurations.

```sh
# Original upstream MQTT demo
MXChip/AZ3166/scripts/build.sh mqtt rebuild

# uProtocol MQTT 5 demo (run this command when you want to deploy uProtocol)
MXChip/AZ3166/scripts/build.sh uprotocol-mqtt rebuild
```

Both commands produce `MXChip/AZ3166/build/app/mxchip_threadx.bin`.
Subsequent builds of the same configuration can omit `rebuild`.
The original MQTT app has its upstream defaults in `app/mqtt/cloud_config.h`.
The uProtocol app's Wi-Fi and broker settings are in
`app/uprotocol-mqtt/cloud_config.h`; its demo identities are in
`app/uprotocol-mqtt/uprotocol_config.h`.

The imported WICED archive uses fixed offsets in NetX's `NX_IP` structure.
Keep `NX_ENABLE_EXTENDED_NOTIFY_SUPPORT` disabled in `lib/netxduo/nx_user.h`;
`app/common/wwd_networking.c` checks those offsets at compile time.

## Deploy

Attach the AZ3166 to WSL with `usbipd` and mount its drive at `/mnt/AZ3166`.
The existing deployment script copies the binary from the standard build
folder:

```sh
sudo MXChip/AZ3166/scripts/deploy.sh /mnt/AZ3166
```

Check its output for a copy error: the script can print success even when
`cp` fails. Wait for DAPLink to remount its drive before resetting the board.
`FAIL.TXT` on the remounted drive indicates a programming failure.
If the board disappears from WSL after a USB reset, attach it again with
`usbipd` before the next deployment.

For serial output, open `sudo screen /dev/ttyACM0 115200` and reset the
board. Exit `screen` with `Ctrl-A`, `K`, then `Y`.

The original MQTT demo uses its configured raw MQTT topics. The uProtocol
app publishes telemetry to `az3166/AB/0/1/8001` and receives powertrain
events on `powertrain/BC/0/1/8001`, with attributes in MQTT 5 properties.
See [UPROTOCOL.adoc](UPROTOCOL.adoc) for its peer contract and host tests.
