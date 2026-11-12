import test from 'node:test';
import assert from 'node:assert/strict';
import { EventEmitter } from 'node:events';
import { PassThrough } from 'node:stream';
import { BazUpgradeClient } from './index.mjs';

function createClientProcessFixture() {
  const child = new EventEmitter();
  child.stdin = new PassThrough();
  child.stdout = new PassThrough();
  child.stderr = new PassThrough();
  const spawnCalls = [];
  let unreferenced = false;

  child.unref = () => {
    unreferenced = true;
  };

  const client = new BazUpgradeClient(
    { executable: process.execPath, config: process.execPath },
    (...arguments_) => {
      spawnCalls.push(arguments_);
      return child;
    }
  );

  return { child, spawnCalls, client, wasUnreferenced: () => unreferenced };
}
test('paths and command names are constrained', () => {
  assert.throws(() => new BazUpgradeClient({ executable: 'relative', config: 'relative' }));
  const { client } = createClientProcessFixture();

  assert.throws(() => client.run('arbitrary-command'));
  assert.throws(() => client.apply({ waitPid: '1; other-command' }));
});
test('JSON results use arguments without a shell', async () => {
  const { child, spawnCalls, client } = createClientProcessFixture();

  const result = client.check();
  child.stdout.write('{"status":"upToDate"}\n');
  child.emit('close', 0);

  assert.equal((await result).status, 'upToDate');
  assert.equal(spawnCalls[0][2].shell, undefined);
  assert.ok(spawnCalls[0][1].includes('--control-stdin'));
});
test('cancellation remains owned until process close', async () => {
  const { child, client } = createClientProcessFixture();
  const abort = new AbortController();
  let control = '';
  let settled = false;

  child.stdin.on('data', value => {
    control += value;
  });
  const result = client.apply({ signal: abort.signal }).finally(() => {
    settled = true;
  });

  abort.abort();
  await new Promise(resolve => setImmediate(resolve));

  assert.equal(control, 'cancel\n');
  assert.equal(settled, false);

  child.stderr.write('{"status":"error","message":"cancelled"}\n');
  child.emit('close', 7);
  await assert.rejects(result, /cancelled/);
});
test('self-update does not permit quit before verified readiness', async () => {
  const { child, client, wasUnreferenced } = createClientProcessFixture();
  let settled = false;

  const result = client.prepareSelfUpdate(1234).then(readiness => {
    settled = true;
    return readiness;
  });

  child.emit('spawn');
  await new Promise(resolve => setImmediate(resolve));

  assert.equal(settled, false);
  child.stdout.write('{"event":"rea');
  child.stdout.write('dy"}\n');

  assert.deepEqual(await result, { updateStarted: true });
  assert.equal(wasUnreferenced(), true);
});
test('self-update handles no-update without requesting quit', async () => {
  const { child, client } = createClientProcessFixture();

  const result = client.prepareSelfUpdate(1234);
  child.stdout.write('{"status":"upToDate"}\n');
  child.emit('close', 0);

  assert.deepEqual(await result, { updateStarted: false });
});
test('Electron verification rejects unsupported versions and companions', async () => {
  const { child, client } = createClientProcessFixture();
  const updater = { verifyUpdateFile: null };

  assert.throws(() => client.attachElectronUpdater({}));
  client.attachElectronUpdater(updater);
  assert.equal((await updater.verifyUpdateFile({ packageFilePath: 'companion' })).response, 'failure');

  const verified = updater.verifyUpdateFile({ updateFilePath: process.execPath });
  child.stdout.write('{"status":"verified"}\n');
  child.emit('close', 0);

  assert.equal((await verified).response, 'success');
});
