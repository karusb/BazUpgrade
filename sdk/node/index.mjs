import { spawn } from 'node:child_process';
import { isAbsolute } from 'node:path';

export class BazUpgradeClient {
  constructor({ executable, config }, spawnProcess = spawn) {
    if (!isAbsolute(executable) || !isAbsolute(config)) {
      throw new TypeError('executable and config must be absolute trusted local paths');
    }
    this.executable = executable;
    this.config = config;
    this.spawnProcess = spawnProcess;
  }

  run(command, options = {}) {
    const allowed = new Set(['check', 'apply', 'status', 'verify', 'reconcile', 'rollback']);
    if (!allowed.has(command)) throw new TypeError('Unknown updater command');
    if (options.waitPid !== undefined && (!Number.isSafeInteger(options.waitPid) || options.waitPid < 1 || options.waitPid > 2147483647)) {
      throw new TypeError('Invalid application PID');
    }
    const args = [command, '--config', this.config, '--control-stdin'];
    if (options.file) args.push('--file', options.file);
    if (options.waitPid) args.push('--wait-pid', String(options.waitPid));
    if (options.installed !== undefined) args.push('--installed', String(options.installed));
    return new Promise((resolve, reject) => {
      if (options.signal?.aborted) return reject(new Error('Update operation cancelled'));
      const child = this.spawnProcess(this.executable, args, { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
      let stdout = '', stderr = '', overflow = false, spawnError;
      child.stdout.setEncoding('utf8');
      child.stderr.setEncoding('utf8');
      child.stdin.on('error', () => {});
      const cancel = () => {
        if (!child.stdin.destroyed) child.stdin.write('cancel\n');
      };
      options.signal?.addEventListener('abort', cancel, { once: true });
      const receive = (name, chunk) => {
        if (Buffer.byteLength(stdout) + Buffer.byteLength(stderr) + Buffer.byteLength(chunk) > 1024 * 1024) {
          overflow = true;
          cancel();
          return;
        }
        if (name === 'stdout') stdout += chunk;
        else stderr += chunk;
      };
      child.stdout.on('data', chunk => receive('stdout', chunk));
      child.stderr.on('data', chunk => receive('stderr', chunk));
      child.on('error', error => { spawnError = error; });
      // Cancellation is cooperative; native work retains ownership through process close.
      child.on('close', code => {
        options.signal?.removeEventListener('abort', cancel);
        child.stdin.end();
        if (spawnError) return reject(spawnError);
        if (overflow) return reject(new Error('Updater output exceeded its limit'));
        try {
          const result = JSON.parse(code === 0 ? stdout : stderr);
          if (code !== 0) return reject(Object.assign(new Error(result.message), { result, exitCode: code }));
          resolve(result);
        } catch (error) { reject(error); }
      });
    });
  }

  check(options) { return this.run('check', options); }
  apply(options) { return this.run('apply', options); }
  status() { return this.run('status'); }

  prepareSelfUpdate(pid = process.pid) {
    if (!Number.isSafeInteger(pid) || pid < 1 || pid > 2147483647) throw new TypeError('Invalid application PID');
    return new Promise((resolve, reject) => {
      const child = this.spawnProcess(this.executable, [
        'apply', '--config', this.config, '--wait-pid', String(pid),
        '--notify-ready', '--control-stdin',
      ], { detached: true, windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
      let pending = '', stderr = '', ready = false, spawnError;
      child.stdout.setEncoding('utf8');
      child.stderr.setEncoding('utf8');
      child.stdin.on('error', () => {});
      child.stderr.on('data', chunk => { if (stderr.length < 65536) stderr += chunk; });
      child.stdout.on('data', chunk => {
        if (ready) return;
        pending += chunk;
        if (pending.length > 65536) { child.stdin.write('cancel\n'); return; }
        const newline = pending.indexOf('\n');
        if (newline < 0) return;
        try {
          const message = JSON.parse(pending.slice(0, newline));
          if (message.event === 'ready') {
            ready = true;
            child.stdin.end();
            child.stdout.destroy();
            child.stderr.destroy();
            child.unref();
            resolve({ updateStarted: true });
          }
        } catch { child.stdin.write('cancel\n'); }
      });
      child.on('error', error => { spawnError = error; });
      child.on('close', code => {
        if (ready) return;
        if (spawnError) return reject(spawnError);
        try {
          const message = JSON.parse(code === 0 ? pending : stderr);
          if (code === 0 && message.status === 'upToDate') return resolve({ updateStarted: false });
          reject(new Error(message.message || 'Updater did not become ready'));
        } catch (error) { reject(error); }
      });
    });
  }

  attachElectronUpdater(updater) {
    if (!('verifyUpdateFile' in updater)) throw new Error('This electron-updater version has no downloaded-file verification hook');
    updater.verifyUpdateFile = async ({ updateFilePath, packageFilePath }) => {
      try {
        if (packageFilePath) throw new Error('Companion packages require separate signed targets; use a full installer');
        await this.run('verify', { file: updateFilePath });
        return { response: 'success' };
      } catch (error) {
        return { response: 'failure', message: error.message };
      }
    };
  }
}
