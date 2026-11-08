import test from 'node:test';
import assert from 'node:assert/strict';
import { EventEmitter } from 'node:events';
import { PassThrough } from 'node:stream';
import { BazUpgradeClient } from './index.mjs';

function fixture() {
  const child = new EventEmitter();
  child.stdin = new PassThrough();
  child.stdout = new PassThrough();
  child.stderr = new PassThrough();
  let unreferenced = false;
  child.unref = () => { unreferenced = true; };
  const calls = [];
  const client = new BazUpgradeClient({ executable: process.execPath, config: process.execPath }, (...args) => {
    calls.push(args);
    return child;
  });
  return { child, calls, client, unreferenced: () => unreferenced };
}
test('paths and command names are constrained', () => {
  assert.throws(() => new BazUpgradeClient({ executable: 'relative', config: 'relative' }));
  const { client } = fixture();
  assert.throws(() => client.run('arbitrary-command'));
  assert.throws(() => client.apply({ waitPid: '1; other-command' }));
});
test('JSON results use arguments without a shell', async () => {
  const { child, calls, client } = fixture();
  const result = client.check();
  child.stdout.write('{"status":"upToDate"}\n');
  child.emit('close', 0);
  assert.equal((await result).status, 'upToDate');
  assert.equal(calls[0][2].shell, undefined);
  assert.ok(calls[0][1].includes('--control-stdin'));
});
test('cancellation remains owned until process close', async () => {
  const { child, client } = fixture();
  const abort = new AbortController();
  let control = '';
  child.stdin.on('data', value => { control += value; });
  let settled = false;
  const result = client.apply({ signal: abort.signal }).finally(() => { settled = true; });
  abort.abort();
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(control, 'cancel\n');
  assert.equal(settled, false);
  child.stderr.write('{"status":"error","message":"cancelled"}\n');
  child.emit('close', 7);
  await assert.rejects(result, /cancelled/);
});
test('self-update does not permit quit before verified readiness', async () => {
  const value = fixture();
  let settled = false;
  const result = value.client.prepareSelfUpdate(1234).then(value => { settled = true; return value; });
  value.child.emit('spawn');
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(settled, false);
  value.child.stdout.write('{"event":"rea');
  value.child.stdout.write('dy"}\n');
  assert.deepEqual(await result, { updateStarted: true });
  assert.equal(value.unreferenced(), true);
});
test('self-update handles no-update without requesting quit', async () => {
  const { child, client } = fixture();
  const result = client.prepareSelfUpdate(1234);
  child.stdout.write('{"status":"upToDate"}\n');
  child.emit('close', 0);
  assert.deepEqual(await result, { updateStarted: false });
});
test('Electron verification rejects unsupported versions and companions', async () => {
  const { child, client } = fixture();
  assert.throws(() => client.attachElectronUpdater({}));
  const updater = { verifyUpdateFile: null };
  client.attachElectronUpdater(updater);
  assert.equal((await updater.verifyUpdateFile({ packageFilePath: 'companion' })).response, 'failure');
  const verified = updater.verifyUpdateFile({ updateFilePath: process.execPath });
  child.stdout.write('{"status":"verified"}\n');
  child.emit('close', 0);
  assert.equal((await verified).response, 'success');
});
