# Node / Electron main-process SDK

No npm runtime dependencies are required.

~~~js
import { BazUpgradeClient } from './index.mjs';
const updates = new BazUpgradeClient({
  executable: '/absolute/path/to/bazupgrade',
  config: '/absolute/path/to/client.json',
});
const result = await updates.check();
~~~

Keep this module in the desktop/main process. Do not accept arbitrary executable/configuration paths from a renderer.

## Separate update manager

~~~js
const abort = new AbortController();
const result = await updates.apply({ signal: abort.signal });
~~~

Cancellation retains process ownership until close. An installer already applying changes is observed until exit.

## Self-update

~~~js
const prepared = await updates.prepareSelfUpdate();
if (prepared.updateStarted) {
  // Persist application state, then quit normally.
  app.quit();
}
~~~

The helper reports readiness after verification and waits for the old process. Observe final installation status on the next launch.

## Existing electron-updater

~~~js
updates.attachElectronUpdater(autoUpdater);
~~~

Use a verification-only electron target. A compatible downloaded-file verification hook is required; native signature verification remains active. Companion web-installer packages are rejected.

See [integration boundaries](../../docs/integrations.md).
