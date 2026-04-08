<!--
  Copyright (c) 2026 Eclipse Foundation
 
  This program and the accompanying materials are made available 
  under the terms of the MIT license which is available at
  https://opensource.org/license/mit.
 
  SPDX-License-Identifier: MIT
 
  Contributors: 
      Frédéric Desbiens - Initial version.

-->

# Eclipse ThreadX Sample Applications

This repository will contain up-to-date samples for several development boards tracking the latest ThreadX releases. 

For the time being, it hosts a restructured, self-contained version of the application available in the [iot-devkit](https://github.com/eclipse-threadx/iot-devkit) repository. 

## Cloning this repository
Eclipse ThreadX, Eclipse ThreadX NetX Duo, Eclipse ThreadX USBX, and Eclipse FileX are included as submodules.

When cloning, you must specify the `--recurse-submodules` option to get the code for the submodules. If you forget this option, just run the following commands in the root folder of your clone. 

```
git submodule init
git submodule update --recursive
```
