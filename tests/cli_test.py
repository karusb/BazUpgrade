"""CLI, C ABI, loopback transport and reference TUF interoperability."""
import ctypes
import functools
import http.server
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
import xml.etree.ElementTree as ET

UPDATER, PUBLISHER, SDK = map(lambda value: str(Path(value).resolve()), sys.argv[1:4])
NATIVE_FIXTURE = str(Path(sys.argv[4]).resolve()) if len(sys.argv) > 4 else None
sys.argv = sys.argv[:1]

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass

class Integration(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="bazupgrade-cli-")
        self.root = Path(self.temporary.name).resolve() / "space unicode \u03bb"
        self.root.mkdir()
        self.repo = self.root / "repository"
        self.keys = self.root / "private"
        self.publisher("init")
        self.bootstrap = self.root / "bootstrap.json"
        shutil.copyfile(self.repo / "metadata" / "1.root.json", self.bootstrap)
        application = self.root / "application"
        application.mkdir()
        (application / "hello.txt").write_text("synthetic application\n", encoding="utf-8")
        self.package = self.root / "application.zip"
        self.call(PUBLISHER, "pack", "--directory", str(application), "--output", str(self.package))
        self.descriptor = {
            "schemaVersion": 1, "appId": "example", "channel": "stable",
            "os": "test", "arch": "x64", "packageType": "portable",
            "appVersion": "1.9.0", "releaseSequence": 1, "entrypoint": "hello.txt",
            "notes": "Example, update\nUnicode \u03bb",
        }
        self.release_file = self.root / "release.json"
        self.publish()
        self.config_file = self.root / "client.json"
        self.config = {
            "schemaVersion": 1, "repository": str(self.repo), "trustedRoot": str(self.bootstrap),
            "appId": "example", "channel": "stable", "os": "test", "arch": "x64",
            "packageType": "portable", "target": "example.zip",
            "stateDirectory": str(self.root / "state"), "installDirectory": str(self.root / "installed"),
        }
        self.save_config()

    def tearDown(self):
        self.temporary.cleanup()

    def call(self, executable, *args, success=True):
        result = subprocess.run([executable, *args], capture_output=True, text=True, encoding="utf-8", timeout=45)
        if success:
            self.assertEqual(result.returncode, 0, result.stderr)
            return json.loads(result.stdout)
        self.assertNotEqual(result.returncode, 0, result.stdout)
        return json.loads(result.stderr)

    def publisher(self, command, *args, **kwargs):
        return self.call(PUBLISHER, command, "--repo", str(self.repo), "--keys", str(self.keys), *args, **kwargs)

    def publish(self):
        self.release_file.write_text(json.dumps(self.descriptor), encoding="utf-8")
        return self.publisher("publish", "--artifact", str(self.package), "--target", "example.zip", "--descriptor", str(self.release_file))

    def save_config(self):
        self.config_file.write_text(json.dumps(self.config), encoding="utf-8")

    def client(self, command, *args, **kwargs):
        return self.call(UPDATER, command, "--config", str(self.config_file), *args, **kwargs)

    def server(self):
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(QuietHandler, directory=str(self.repo)))
        server.daemon_threads = True
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        return "http://127.0.0.1:" + str(server.server_port)

    def test_cli_install_and_no_replay(self):
        self.assertEqual(self.client("check")["status"], "updateAvailable")
        self.assertEqual(self.client("apply")["status"], "updated")
        state = self.client("status")
        self.assertEqual(Path(state["activePath"]).read_text(), "synthetic application\n")
        self.assertEqual(self.client("apply")["status"], "upToDate")

    def test_option_contracts_reject_invalid_input_before_side_effects(self):
        for arguments in (
            ("apply", "--config", str(self.config_file), "--config", str(self.config_file)),
            ("apply", "--config", str(self.config_file), "-c", str(self.config_file)),
            ("apply", "--config", str(self.config_file), "--wait-pid", "--notify-ready"),
            ("apply", "--config", str(self.config_file), "--wait-pid"),
            ("verify", "--config", str(self.config_file), "--file"),
            ("verify", "--config", str(self.config_file), "--file", "--unknown"),
            ("capabilities", "--config", str(self.config_file)),
            ("status", "--config", str(self.config_file), "--wait-pid", "0"),
        ):
            with self.subTest(arguments=arguments):
                self.assertEqual(self.call(UPDATER, *arguments, success=False)["status"], "error")
        unopened = self.root / "must-not-be-created"
        for arguments in (
            ("--repo", str(unopened), "--keys", str(self.root / "unopened-keys")),
            ("--repo", str(unopened), "--repo", str(unopened), "--keys", str(self.root / "unopened-keys")),
            ("--repo", str(unopened), "--keys"),
        ):
            with self.subTest(publisher_arguments=arguments):
                self.call(PUBLISHER, "publish", *arguments, success=False)
        self.assertFalse(unopened.exists())
        self.assertFalse((self.root / "unopened-keys").exists())
        self.assertFalse((self.root / "state").exists())

    def test_explicit_loopback_http(self):
        self.config["repository"] = self.server()
        self.save_config()
        self.client("check", success=False)
        self.config["allowLoopbackHttp"] = True
        self.save_config()
        self.assertEqual(self.client("apply")["status"], "updated")

    def test_http_download_encodes_raw_target_path_segments(self):
        target = "nested/a b\u03bb.zip"
        self.publisher("publish", "--artifact", str(self.package), "--target", target,
                       "--descriptor", str(self.release_file))
        self.config["repository"] = self.server()
        self.config["allowLoopbackHttp"] = True
        self.config["target"] = target
        self.save_config()
        self.assertEqual(self.client("apply")["status"], "updated")
        active = Path(self.client("status")["activePath"])
        self.assertEqual(active.read_text(), "synthetic application\n")

    def test_c_abi_owns_output(self):
        library = ctypes.CDLL(SDK)
        library.baz_client_create.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_void_p)]
        library.baz_client_run.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p)]
        library.baz_client_destroy.argtypes = [ctypes.c_void_p]
        library.baz_string_free.argtypes = [ctypes.c_void_p]
        handle, output = ctypes.c_void_p(), ctypes.c_void_p()
        self.assertEqual(library.baz_client_create(str(self.config_file).encode(), ctypes.byref(handle), ctypes.byref(output)), 0)
        try:
            self.assertEqual(library.baz_client_run(handle, b"check", None, ctypes.byref(output)), 0)
            self.assertEqual(json.loads(ctypes.string_at(output))["status"], "updateAvailable")
            library.baz_string_free(output)
            output = ctypes.c_void_p()
            self.assertNotEqual(library.baz_client_run(handle, b"unknown", None, ctypes.byref(output)), 0)
            self.assertEqual(json.loads(ctypes.string_at(output))["status"], "error")
            library.baz_string_free(output)
        finally:
            library.baz_client_destroy(handle)

    def test_reference_python_tuf_downloads_publisher_output(self):
        try:
            from tuf.ngclient import Updater
        except ImportError:
            if os.environ.get("BAZUPGRADE_REQUIRE_REFERENCE_TUF") == "1":
                self.fail("Reference python-tuf is required for this CI job")
            self.skipTest("python-tuf is not installed")
        base = self.server()
        metadata = self.root / "reference-metadata"
        downloads = self.root / "reference-downloads"
        metadata.mkdir()
        downloads.mkdir()
        shutil.copyfile(self.bootstrap, metadata / "root.json")
        import inspect
        arguments = {}
        if "bootstrap" in inspect.signature(Updater).parameters:
            arguments["bootstrap"] = self.bootstrap.read_bytes()
        updater = Updater(str(metadata), base + "/metadata/", str(downloads), base + "/targets/", **arguments)
        updater.refresh()
        info = updater.get_targetinfo("example.zip")
        self.assertIsNotNone(info)
        downloaded = updater.download_target(info)
        self.assertEqual(Path(downloaded).read_bytes(), self.package.read_bytes())

    def test_refresh_and_root_rotation(self):
        self.client("check")
        self.publisher("rotate-root")
        self.publisher("refresh")
        self.assertEqual(self.client("check")["status"], "updateAvailable")
        self.assertEqual(json.loads((self.root / "state/trust/root.json").read_text())["signed"]["version"], 2)

    def test_native_feed_export_is_explicit(self):
        output = self.root / "latest.yml"
        self.publisher("export-electron", "--target", "example.zip", "--base-url", "https://updates.example.com/", "--output", str(output), success=False)
        result = self.publisher("export-electron", "--target", "example.zip", "--base-url", "https://updates.example.com/", "--output", str(output), "--unsigned-native-feed", "true")
        self.assertFalse(result["tufEnforcedByNativeFeed"])
        self.assertEqual(json.loads(output.read_text())["version"], "1.9.0")

    def test_appinstaller_export_escapes_identity(self):
        self.descriptor["packageType"] = "msix"
        self.descriptor["releaseSequence"] = 2
        self.descriptor["nativeIdentity"] = {
            "name": "Example.App", "publisher": "CN=Example & Co",
            "version": "1.0.0.0", "processorArchitecture": "x64",
        }
        self.publish()
        output = self.root / "example.appinstaller"
        self.publisher("export-appinstaller", "--target", "example.zip", "--base-url", "https://updates.example.com/", "--output", str(output))
        document = ET.parse(output)
        package = list(document.getroot())[0]
        self.assertEqual(package.attrib["Publisher"], "CN=Example & Co")

    @unittest.skipUnless(os.name == "nt" and NATIVE_FIXTURE, "Windows native subprocess contract")
    def test_native_installer_progress_does_not_corrupt_json(self):
        self.package = Path(NATIVE_FIXTURE)
        self.descriptor["packageType"] = "nsis"
        self.descriptor["releaseSequence"] = 2
        self.publish()
        self.config["packageType"] = "nsis"
        self.save_config()
        self.assertEqual(self.client("apply")["status"], "updated")
        self.assertIn("synthetic installer progress", (self.root / "installed/native-install.log").read_text())

if __name__ == "__main__":
    unittest.main(verbosity=2)
