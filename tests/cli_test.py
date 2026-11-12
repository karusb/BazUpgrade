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
import urllib.parse
import xml.etree.ElementTree as ET

UPDATER, PUBLISHER, SDK = map(lambda value: str(Path(value).resolve()), sys.argv[1:4])
NATIVE_FIXTURE = str(Path(sys.argv[4]).resolve()) if len(sys.argv) > 4 else None
sys.argv = sys.argv[:1]

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass

class UpdateToolchainE2E(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="bazupgrade-cli-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve() / "space unicode \u03bb"
        self.root.mkdir()

        self.repo = self.root / "repository"
        self.keys = self.root / "private"
        self.bootstrap = self.root / "bootstrap.json"
        self.package = self.root / "application.zip"
        self.release_file = self.root / "release.json"
        self.config_file = self.root / "client.json"

        self.initialize_signed_repository()
        self.create_application_archive()
        self.descriptor = self.build_release_descriptor()
        self.publish_release()
        self.config = self.build_client_configuration()
        self.write_client_configuration()

    def initialize_signed_repository(self):
        self.run_publisher_command("init")
        shutil.copyfile(self.repo / "metadata" / "1.root.json", self.bootstrap)

    def create_application_archive(self):
        application = self.root / "application"

        application.mkdir()
        (application / "hello.txt").write_text(
            "synthetic application\n", encoding="utf-8"
        )
        self.run_json_command(
            PUBLISHER, "pack", "--directory", str(application),
            "--output", str(self.package),
        )

    def build_release_descriptor(self):
        return {
            "schemaVersion": 1,
            "appId": "example",
            "channel": "stable",
            "os": "test",
            "arch": "x64",
            "packageType": "portable",
            "appVersion": "1.9.0",
            "releaseSequence": 1,
            "entrypoint": "hello.txt",
            "notes": "Example, update\nUnicode \u03bb",
        }

    def build_client_configuration(self):
        return {
            "schemaVersion": 1,
            "repository": str(self.repo),
            "trustedRoot": str(self.bootstrap),
            "appId": "example",
            "channel": "stable",
            "os": "test",
            "arch": "x64",
            "packageType": "portable",
            "target": "example.zip",
            "stateDirectory": str(self.root / "state"),
            "installDirectory": str(self.root / "installed"),
        }

    def run_json_command(self, executable, *arguments, success=True):
        result = subprocess.run(
            [executable, *arguments], capture_output=True,
            text=True, encoding="utf-8", timeout=45,
        )

        if success:
            self.assertEqual(result.returncode, 0, result.stderr)
            return json.loads(result.stdout)

        self.assertNotEqual(result.returncode, 0, result.stdout)
        return json.loads(result.stderr)

    def run_publisher_command(self, command, *arguments, **options):
        return self.run_json_command(
            PUBLISHER, command, "--repo", str(self.repo),
            "--keys", str(self.keys), *arguments, **options,
        )

    def run_update_command(self, command, *arguments, **options):
        return self.run_json_command(
            UPDATER, command, "--config", str(self.config_file),
            *arguments, **options,
        )

    def publish_release(self):
        self.release_file.write_text(json.dumps(self.descriptor), encoding="utf-8")

        return self.run_publisher_command(
            "publish", "--artifact", str(self.package), "--target", "example.zip",
            "--descriptor", str(self.release_file),
        )

    def write_client_configuration(self):
        self.config_file.write_text(json.dumps(self.config), encoding="utf-8")

    def start_repository_server(self):
        handler = functools.partial(QuietHandler, directory=str(self.repo))
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)

        server.daemon_threads = True
        thread.start()
        self.addCleanup(self.stop_repository_server, server, thread)

        return f"http://127.0.0.1:{server.server_port}"

    @staticmethod
    def stop_repository_server(server, thread):
        server.shutdown()
        server.server_close()
        thread.join()

    @staticmethod
    def load_sdk_library():
        library = ctypes.CDLL(SDK)
        pointer = ctypes.c_void_p
        output_pointer = ctypes.POINTER(pointer)

        library.baz_client_create.argtypes = [ctypes.c_char_p, output_pointer, output_pointer]
        library.baz_client_create.restype = ctypes.c_int
        library.baz_client_run.argtypes = [
            pointer, ctypes.c_char_p, ctypes.c_char_p, output_pointer,
        ]
        library.baz_client_run.restype = ctypes.c_int
        library.baz_client_destroy.argtypes = [pointer]
        library.baz_client_destroy.restype = None
        library.baz_string_free.argtypes = [pointer]
        library.baz_string_free.restype = None

        return library

    def assert_sdk_command_result(self, library, handle, command, status, success):
        output = ctypes.c_void_p()

        try:
            result = library.baz_client_run(handle, command, None, ctypes.byref(output))

            if success:
                self.assertEqual(result, 0)
            else:
                self.assertNotEqual(result, 0)
            self.assertTrue(output.value)
            self.assertEqual(json.loads(ctypes.string_at(output))["status"], status)
        finally:
            if output.value:
                library.baz_string_free(output)

    def test_cli_install_and_no_replay(self):
        self.assertEqual(self.run_update_command("check")["status"], "updateAvailable")
        self.assertEqual(self.run_update_command("apply")["status"], "updated")
        state = self.run_update_command("status")
        self.assertEqual(Path(state["activePath"]).read_text(), "synthetic application\n")
        self.assertEqual(self.run_update_command("apply")["status"], "upToDate")

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
                result = self.run_json_command(UPDATER, *arguments, success=False)

                self.assertEqual(result["status"], "error")
        unopened = self.root / "must-not-be-created"
        for arguments in (
            ("--repo", str(unopened), "--keys", str(self.root / "unopened-keys")),
            (
                "--repo", str(unopened), "--repo", str(unopened),
                "--keys", str(self.root / "unopened-keys"),
            ),
            ("--repo", str(unopened), "--keys"),
        ):
            with self.subTest(publisher_arguments=arguments):
                self.run_json_command(PUBLISHER, "publish", *arguments, success=False)
        self.assertFalse(unopened.exists())
        self.assertFalse((self.root / "unopened-keys").exists())
        self.assertFalse((self.root / "state").exists())

    def test_explicit_loopback_http(self):
        self.config["repository"] = self.start_repository_server()
        self.write_client_configuration()
        self.run_update_command("check", success=False)
        self.config["allowLoopbackHttp"] = True
        self.write_client_configuration()
        self.assertEqual(self.run_update_command("apply")["status"], "updated")

    def test_http_download_encodes_raw_target_path_segments(self):
        target = "nested/a b\u03bb.zip"
        self.run_publisher_command("publish", "--artifact", str(self.package), "--target", target,
                       "--descriptor", str(self.release_file))
        self.config["repository"] = self.start_repository_server()
        self.config["allowLoopbackHttp"] = True
        self.config["target"] = target
        self.write_client_configuration()
        self.assertEqual(self.run_update_command("apply")["status"], "updated")
        active = Path(self.run_update_command("status")["activePath"])
        self.assertEqual(active.read_text(), "synthetic application\n")

    def test_c_abi_owns_output(self):
        library = self.load_sdk_library()
        handle = ctypes.c_void_p()
        error = ctypes.c_void_p()

        try:
            result = library.baz_client_create(
                str(self.config_file).encode("utf-8"), ctypes.byref(handle),
                ctypes.byref(error),
            )

            self.assertEqual(result, 0)
            self.assertTrue(handle.value)
            self.assertFalse(error.value)
            self.assert_sdk_command_result(library, handle, b"check", "updateAvailable", True)
            self.assert_sdk_command_result(library, handle, b"unknown", "error", False)
        finally:
            if error.value:
                library.baz_string_free(error)
            library.baz_client_destroy(handle)

    def test_reference_python_tuf_downloads_publisher_output(self):
        try:
            from tuf.ngclient import Updater
        except ImportError:
            if os.environ.get("BAZUPGRADE_REQUIRE_REFERENCE_TUF") == "1":
                self.fail("Reference python-tuf is required for this CI job")
            self.skipTest("python-tuf is not installed")
        base = self.start_repository_server()
        metadata = self.root / "reference-metadata"
        downloads = self.root / "reference-downloads"
        metadata.mkdir()
        downloads.mkdir()
        shutil.copyfile(self.bootstrap, metadata / "root.json")
        import inspect
        arguments = {}
        if "bootstrap" in inspect.signature(Updater).parameters:
            arguments["bootstrap"] = self.bootstrap.read_bytes()
        updater = Updater(
            str(metadata), base + "/metadata/", str(downloads),
            base + "/targets/", **arguments,
        )
        updater.refresh()
        info = updater.get_targetinfo("example.zip")
        self.assertIsNotNone(info)
        downloaded = updater.download_target(info)
        self.assertEqual(Path(downloaded).read_bytes(), self.package.read_bytes())

    def test_refresh_and_root_rotation(self):
        self.run_update_command("check")
        self.run_publisher_command("rotate-root")
        self.run_publisher_command("refresh")
        self.assertEqual(self.run_update_command("check")["status"], "updateAvailable")
        root_file = self.root / "state/trust/root.json"
        trusted_root = json.loads(root_file.read_text(encoding="utf-8"))

        self.assertEqual(trusted_root["signed"]["version"], 2)

    def test_native_feed_export_is_explicit(self):
        output = self.root / "latest.yml"
        target = "nested/a b\u03bb.zip"

        self.run_publisher_command(
            "publish", "--artifact", str(self.package), "--target", target,
            "--descriptor", str(self.release_file),
        )
        arguments = (
            "export-electron", "--target", target, "--base-url",
            "https://updates.example.com/", "--output", str(output),
        )

        self.run_publisher_command(*arguments, success=False)
        result = self.run_publisher_command(*arguments, "--unsigned-native-feed", "true")

        self.assertFalse(result["tufEnforcedByNativeFeed"])
        feed = json.loads(output.read_text(encoding="utf-8"))
        uri = urllib.parse.urlsplit(feed["path"])

        self.assertEqual(feed["version"], "1.9.0")
        self.assertEqual(uri.fragment, "")
        self.assertNotIn(" ", feed["path"])
        self.assertTrue(feed["path"].isascii())
        self.assertTrue(urllib.parse.unquote(uri.path).endswith(".a b\u03bb.zip"))

    def test_appinstaller_export_escapes_identity(self):
        self.descriptor["packageType"] = "msix"
        self.descriptor["releaseSequence"] = 2
        self.descriptor["nativeIdentity"] = {
            "name": "Example.App", "publisher": "CN=Example & Co",
            "version": "1.0.0.0", "processorArchitecture": "x64",
        }
        self.publish_release()
        output = self.root / "example \u03bb#.appinstaller"
        self.run_publisher_command(
            "export-appinstaller", "--target", "example.zip", "--base-url",
            "https://updates.example.com/", "--output", str(output),
        )
        document = ET.parse(output)
        package = list(document.getroot())[0]
        self.assertEqual(package.attrib["Publisher"], "CN=Example & Co")

        manifest_uri = urllib.parse.urlsplit(document.getroot().attrib["Uri"])

        self.assertEqual(manifest_uri.fragment, "")
        self.assertEqual(urllib.parse.unquote(manifest_uri.path), "/" + output.name)

    @unittest.skipUnless(os.name == "nt" and NATIVE_FIXTURE, "Windows native subprocess contract")
    def test_native_installer_progress_does_not_corrupt_json(self):
        self.package = Path(NATIVE_FIXTURE)
        self.descriptor["packageType"] = "nsis"
        self.descriptor["releaseSequence"] = 2
        self.publish_release()
        self.config["packageType"] = "nsis"
        self.write_client_configuration()
        self.assertEqual(self.run_update_command("apply")["status"], "updated")
        log = self.root / "installed/native-install.log"

        self.assertIn("synthetic installer progress", log.read_text(encoding="utf-8"))

if __name__ == "__main__":
    unittest.main(verbosity=2)
