#!/usr/bin/env python3

import asyncio
import argparse
from glob import glob
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import sysconfig
from asyncio import wait_for
from contextlib import asynccontextmanager
from os.path import basename, relpath
from pathlib import Path
from subprocess import CalledProcessError, DEVNULL
from tempfile import TemporaryDirectory
from time import sleep


SCRIPT_NAME = Path(__file__).name
CHECKOUT = Path(__file__).resolve().parent.parent
ANDROID_DIR = CHECKOUT / "Android"
TESTBED_DIR = ANDROID_DIR / "testbed"
CROSS_BUILD_DIR = CHECKOUT / "cross-build"

APP_ID = "org.python.testbed"
DECODE_ARGS = ("UTF-8", "backslashreplace")


try:
    android_home = os.environ['ANDROID_HOME']
except KeyError:
    sys.exit("The ANDROID_HOME environment variable is required.")

sdkmanager = Path(
    f"{android_home}/cmdline-tools/latest/bin/sdkmanager"
    + (".bat" if os.name == "nt" else "")
)

adb = Path(
    f"{android_home}/platform-tools/adb"
    + (".exe" if os.name == "nt" else "")
)

gradlew = Path(
    f"{TESTBED_DIR}/gradlew"
    + (".bat" if os.name == "nt" else "")
)

logcat_started = False


def delete_glob(pattern):
    # Path.glob doesn't accept non-relative patterns.
    for path in glob(str(pattern)):
        path = Path(path)
        print(f"Deleting {path} ...")
        if path.is_dir() and not path.is_symlink():
            shutil.rmtree(path)
        else:
            path.unlink()


def subdir(name, *, clean=None):
    path = CROSS_BUILD_DIR / name
    if clean:
        delete_glob(path)
    if not path.exists():
        if clean is None:
            sys.exit(
                f"{path} does not exist. Create it by running the appropriate "
                f"`configure` subcommand of {SCRIPT_NAME}.")
        else:
            path.mkdir(parents=True)
    return path


def run(command, *, host=None, env=None, log=True, **kwargs):
    kwargs.setdefault("check", True)
    if env is None:
        env = os.environ.copy()
    original_env = env.copy()

    if host:
        env_script = ANDROID_DIR / "android-env.sh"
        env_output = subprocess.run(
            f"set -eu; "
            f"HOST={host}; "
            f"PREFIX={subdir(host)}/prefix; "
            f". {env_script}; "
            f"export",
            check=True, shell=True, text=True, stdout=subprocess.PIPE
        ).stdout

        for line in env_output.splitlines():
            # We don't require every line to match, as there may be some other
            # output from installing the NDK.
            if match := re.search(
                "^(declare -x |export )?(\\w+)=['\"]?(.*?)['\"]?$", line
            ):
                key, value = match[2], match[3]
                if env.get(key) != value:
                    print(line)
                    env[key] = value

        if env == original_env:
            raise ValueError(f"Found no variables in {env_script.name} output:\n"
                             + env_output)

    if log:
        print(">", " ".join(map(str, command)))
    return subprocess.run(command, env=env, **kwargs)


def build_python_path():
    """The path to the build Python binary."""
    build_dir = subdir("build")
    binary = build_dir / "python"
    if not binary.is_file():
        binary = binary.with_suffix(".exe")
        if not binary.is_file():
            raise FileNotFoundError("Unable to find `python(.exe)` in "
                                    f"{build_dir}")

    return binary


def configure_build_python(context):
    os.chdir(subdir("build", clean=context.clean))

    command = [relpath(CHECKOUT / "configure")]
    if context.args:
        command.extend(context.args)
    run(command)


def make_build_python(context):
    os.chdir(subdir("build"))
    run(["make", "-j", str(os.cpu_count())])


def unpack_deps(host):
    deps_url = "https://github.com/beeware/cpython-android-source-deps/releases/download"
    for name_ver in ["bzip2-1.0.8-1", "libffi-3.4.4-2", "openssl-3.0.13-1",
                     "sqlite-3.45.1-0", "xz-5.4.6-0"]:
        filename = f"{name_ver}-{host}.tar.gz"
        download(f"{deps_url}/{name_ver}/{filename}")
        run(["tar", "-xf", filename])
        os.remove(filename)


def download(url, target_dir="."):
    out_path = f"{target_dir}/{basename(url)}"
    run(["curl", "-Lf", "-o", out_path, url])
    return out_path


def configure_host_python(context):
    host_dir = subdir(context.host, clean=context.clean)

    prefix_dir = host_dir / "prefix"
    if not prefix_dir.exists():
        prefix_dir.mkdir()
        os.chdir(prefix_dir)
        unpack_deps(context.host)

    build_dir = host_dir / "build"
    build_dir.mkdir(exist_ok=True)
    os.chdir(build_dir)

    command = [
        # Basic cross-compiling configuration
        relpath(CHECKOUT / "configure"),
        f"--host={context.host}",
        f"--build={sysconfig.get_config_var('BUILD_GNU_TYPE')}",
        f"--with-build-python={build_python_path()}",
        "--without-ensurepip",

        # Android always uses a shared libpython.
        "--enable-shared",
        "--without-static-libpython",

        # Dependent libraries. The others are found using pkg-config: see
        # android-env.sh.
        f"--with-openssl={prefix_dir}",
    ]

    if context.args:
        command.extend(context.args)
    run(command, host=context.host)


def make_host_python(context):
    # The CFLAGS and LDFLAGS set in android-env include the prefix dir, so
    # delete any previously-installed Python libs and include files to prevent
    # them being used during the build.
    host_dir = subdir(context.host)
    prefix_dir = host_dir / "prefix"
    delete_glob(f"{prefix_dir}/include/python*")
    delete_glob(f"{prefix_dir}/lib/libpython*")

    os.chdir(host_dir / "build")
    run(["make", "-j", str(os.cpu_count())], host=context.host)
    run(["make", "install", f"prefix={prefix_dir}"], host=context.host)


def build_all(context):
    steps = [configure_build_python, make_build_python, configure_host_python,
             make_host_python]
    for step in steps:
        step(context)


def clean_all(context):
    delete_glob(CROSS_BUILD_DIR)


# To avoid distributing compiled artifacts without corresponding source code,
# the Gradle wrapper is not included in the CPython repository. Instead, we
# extract it from the Gradle release.
def setup_testbed(context):
    ver_long = "8.7.0"
    ver_short = ver_long.removesuffix(".0")

    for filename in ["gradlew", "gradlew.bat"]:
        out_path = download(
            f"https://raw.githubusercontent.com/gradle/gradle/v{ver_long}/{filename}",
            TESTBED_DIR)
        os.chmod(out_path, 0o755)

    with TemporaryDirectory(prefix=SCRIPT_NAME) as temp_dir:
        bin_zip = download(
            f"https://services.gradle.org/distributions/gradle-{ver_short}-bin.zip",
            temp_dir)
        outer_jar = f"gradle-{ver_short}/lib/plugins/gradle-wrapper-{ver_short}.jar"
        run(["unzip", "-d", temp_dir, bin_zip, outer_jar])
        run(["unzip", "-o", "-d", f"{TESTBED_DIR}/gradle/wrapper",
             f"{temp_dir}/{outer_jar}", "gradle-wrapper.jar"])


def setup_testbed_if_needed(context):
    if not gradlew.exists():
        setup_testbed(context)


# run_testbed will build the app automatically, but it hides the Gradle output
# by default, so it's useful to have this as a separate command for the buildbot.
def build_testbed(context):
    setup_testbed_if_needed(context)
    run(
        [gradlew, "--console", "plain", "packageDebug", "packageDebugAndroidTest"],
        cwd=TESTBED_DIR,
    )


# Work around a bug involving sys.exit and TaskGroups
# (https://github.com/python/cpython/issues/101515).
def exit(*args):
    raise MySystemExit(*args)


class MySystemExit(Exception):
    pass


# The `test` subcommand runs all subprocesses through this context manager so
# that no matter what happens, they can always be cancelled from another task,
# and they will always be cleaned up on exit.
@asynccontextmanager
async def async_process(*args, **kwargs):
    process = await asyncio.create_subprocess_exec(*args, **kwargs)
    try:
        yield process
    finally:
        if process.returncode is None:
            # Allow a reasonably long time for Gradle to clean itself up,
            # because we don't want stale emulators left behind.
            timeout = 10
            process.terminate()
            try:
                await wait_for(process.wait(), timeout)
            except TimeoutError:
                print(
                    f"Command {args} did not terminate after {timeout} seconds "
                    f" - sending SIGKILL"
                )
                process.kill()

                # Even after killing the process we must still wait for it,
                # otherwise we'll get the warning "Exception ignored in __del__".
                await wait_for(process.wait(), timeout=1)


async def async_check_output(*args, **kwargs):
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    ) as process:
        stdout, stderr = await process.communicate()
        if process.returncode == 0:
            return stdout.decode(*DECODE_ARGS)
        else:
            raise CalledProcessError(
                process.returncode, args,
                stdout.decode(*DECODE_ARGS), stderr.decode(*DECODE_ARGS)
            )


# Return a list of the serial numbers of connected devices. Emulators will have
# serials of the form "emulator-5678".
async def list_devices():
    serials = []
    header_found = False

    lines = (await async_check_output(adb, "devices")).splitlines()
    for line in lines:
        # Ignore blank lines, and all lines before the header.
        line = line.strip()
        if line == "List of devices attached":
            header_found = True
        elif header_found and line:
            try:
                serial, status = line.split()
            except ValueError:
                raise ValueError(f"failed to parse {line!r}")
            if status == "device":
                serials.append(serial)

    if not header_found:
        raise ValueError(f"failed to parse {lines}")
    return serials


async def find_device(context, initial_devices):
    if context.managed:
        print("Waiting for managed device - this may take several minutes")
        while True:
            new_devices = set(await list_devices()).difference(initial_devices)
            if len(new_devices) == 0:
                await asyncio.sleep(1)
            elif len(new_devices) == 1:
                serial = new_devices.pop()
                print(f"Serial: {serial}")
                return serial
            else:
                exit(f"Found more than one new device: {new_devices}")
    else:
        return context.connected


# Before boot is completed, `pm list` will fail with the error "Can't find
# service: package".
async def boot_completed(serial):
    # No need to show a message if the device is already booted.
    shown_message = False
    while True:
        if (await async_check_output(
            adb, "-s", serial, "shell", "getprop", "sys.boot_completed"
        )).strip() == "1":
            return
        else:
            if not shown_message:
                print("Waiting for boot")
                shown_message = True
            await asyncio.sleep(1)


# An older version of this script in #121595 filtered the logs by UID instead,
# which is more reliable since the PID is shorter-lived. But logcat can't filter
# by UID until API level 31.
async def find_pid(serial):
    print("Waiting for app to start - this may take several minutes")
    while True:
        try:
            pid = (await async_check_output(
                adb, "-s", serial, "shell", "pidof", "-s", APP_ID
            )).strip()
        except CalledProcessError as e:
            # If the app isn't running yet, pidof gives no output, so if there
            # is output, there must have been some other error.
            if e.stdout or e.stderr:
                raise
        else:
            # Some older devices (e.g. Nexus 4) return zero even when no process
            # was found, so check whether we actually got any output.
            if pid:
                print(f"PID: {pid}")
                return pid

        # Loop fairly rapidly to avoid missing a short-lived process.
        await asyncio.sleep(0.2)


async def logcat_task(context, initial_devices):
    # Gradle may need to do some large downloads of libraries and emulator
    # images. This will happen during find_device in --managed mode, or find_pid
    # in --connected mode.
    startup_timeout = 600
    serial = await wait_for(find_device(context, initial_devices), startup_timeout)
    await wait_for(boot_completed(serial), startup_timeout)
    pid = await wait_for(find_pid(serial), startup_timeout)

    args = [adb, "-s", serial, "logcat", "--pid", pid,  "--format", "tag"]
    hidden_output = []
    async with async_process(
        *args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    ) as process:
        while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
            if match := re.fullmatch(r"([A-Z])/(.*)", line, re.DOTALL):
                level, message = match.groups()
            else:
                # If the regex doesn't match, this is probably the second or
                # subsequent line of a multi-line message. Python won't produce
                # such messages, but other components might.
                level, message = None, line

            # Put high-level messages on stderr so they're highlighted in the
            # buildbot logs. This will include Python's own stderr.
            stream = (
                sys.stderr
                if level in ["E", "F"]  # ERROR and FATAL (aka ASSERT)
                else sys.stdout
            )

            # To simplify automated processing of the output, e.g. a buildbot
            # posting a failure notice on a GitHub PR, we strip the level and
            # tag indicators from Python's stdout and stderr.
            for prefix in ["python.stdout: ", "python.stderr: "]:
                if message.startswith(prefix):
                    global logcat_started
                    logcat_started = True
                    stream.write(message.removeprefix(prefix))
                    break
            else:
                if context.verbose:
                    # Non-Python messages add a lot of noise, but they may
                    # sometimes help explain a failure.
                    stream.write(line)
                else:
                    hidden_output.append(line)

        # If the device disconnects while logcat is running, which always
        # happens in --managed mode, some versions of adb return non-zero.
        # Distinguish this from a logcat startup error by checking whether we've
        # received a message from Python yet.
        status = await wait_for(process.wait(), timeout=1)
        if status != 0 and not logcat_started:
            raise CalledProcessError(status, args, "".join(hidden_output))


def stop_app(serial):
    run([adb, "-s", serial, "shell", "am", "force-stop", APP_ID], log=False)


async def gradle_task(context):
    env = os.environ.copy()
    if context.managed:
        task_prefix = context.managed
    else:
        task_prefix = "connected"
        env["ANDROID_SERIAL"] = context.connected

    args = [
        gradlew, "--console", "plain", f"{task_prefix}DebugAndroidTest",
        "-Pandroid.testInstrumentationRunnerArguments.pythonArgs="
        + shlex.join(context.args),
    ]
    hidden_output = []
    try:
        async with async_process(
            *args, cwd=TESTBED_DIR, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        ) as process:
            while line := (await process.stdout.readline()).decode(*DECODE_ARGS):
                if context.verbose:
                    sys.stdout.write(line)
                else:
                    hidden_output.append(line)

            status = await wait_for(process.wait(), timeout=1)
            if status == 0:
                exit(0)
            else:
                raise CalledProcessError(status, args)
    finally:
        # If logcat never started, then something has gone badly wrong, so the
        # user probably wants to see the Gradle output even in non-verbose mode.
        if hidden_output and not logcat_started:
            sys.stdout.write("".join(hidden_output))

        # Gradle does not stop the tests when interrupted.
        if context.connected:
            stop_app(context.connected)


async def run_testbed(context):
    if not adb.exists():
        print("Installing Platform-Tools")
        # Input "y" to accept licenses.
        run([sdkmanager, "platform-tools"], text=True, input="y\n" * 100)

    setup_testbed_if_needed(context)

    if context.managed:
        # In this mode, Gradle will create a device with an unpredictable name.
        # So we save a list of the running devices before starting Gradle, and
        # find_device then waits for a new device to appear.
        initial_devices = await list_devices()
    else:
        # In case the previous shutdown was unclean, make sure the app isn't
        # running, otherwise we might show logs from a previous run. This is
        # unnecessary in --managed mode, because Gradle creates a new emulator
        # every time.
        stop_app(context.connected)
        initial_devices = None

    try:
        async with asyncio.TaskGroup() as tg:
            tg.create_task(logcat_task(context, initial_devices))
            tg.create_task(gradle_task(context))
    except* MySystemExit as e:
        raise SystemExit(*e.exceptions[0].args) from None
    except* CalledProcessError as e:
        # Extract it from the ExceptionGroup so it can be handled by `main`.
        raise e.exceptions[0]


# Handle SIGTERM the same way as SIGINT. This ensures that if we're terminated
# by the buildbot worker, we'll make an attempt to clean up our subprocesses.
def install_signal_handler():
    def signal_handler(*args):
        os.kill(os.getpid(), signal.SIGINT)

    signal.signal(signal.SIGTERM, signal_handler)


def parse_args():
    parser = argparse.ArgumentParser()
    subcommands = parser.add_subparsers(dest="subcommand")
    build = subcommands.add_parser("build", help="Build everything")
    configure_build = subcommands.add_parser("configure-build",
                                             help="Run `configure` for the "
                                             "build Python")
    make_build = subcommands.add_parser("make-build",
                                        help="Run `make` for the build Python")
    configure_host = subcommands.add_parser("configure-host",
                                            help="Run `configure` for Android")
    make_host = subcommands.add_parser("make-host",
                                       help="Run `make` for Android")
    subcommands.add_parser(
        "clean", help="Delete the cross-build directory")

    for subcommand in build, configure_build, configure_host:
        subcommand.add_argument(
            "--clean", action="store_true", default=False, dest="clean",
            help="Delete any relevant directories before building")
    for subcommand in build, configure_host, make_host:
        subcommand.add_argument(
            "host", metavar="HOST",
            choices=["aarch64-linux-android", "x86_64-linux-android"],
            help="Host triplet: choices=[%(choices)s]")
    for subcommand in build, configure_build, configure_host:
        subcommand.add_argument("args", nargs="*",
                                help="Extra arguments to pass to `configure`")

    subcommands.add_parser(
        "setup-testbed", help="Download the testbed Gradle wrapper")
    subcommands.add_parser(
        "build-testbed", help="Build the testbed app")
    test = subcommands.add_parser(
        "test", help="Run the test suite")
    test.add_argument(
        "-v", "--verbose", action="store_true",
        help="Show Gradle output, and non-Python logcat messages")
    device_group = test.add_mutually_exclusive_group(required=True)
    device_group.add_argument(
        "--connected", metavar="SERIAL", help="Run on a connected device. "
        "Connect it yourself, then get its serial from `adb devices`.")
    device_group.add_argument(
        "--managed", metavar="NAME", help="Run on a Gradle-managed device. "
        "These are defined in `managedDevices` in testbed/app/build.gradle.kts.")
    test.add_argument(
        "args", nargs="*", help="Extra arguments for `python -m test`")

    return parser.parse_args()


def main():
    install_signal_handler()
    context = parse_args()
    dispatch = {"configure-build": configure_build_python,
                "make-build": make_build_python,
                "configure-host": configure_host_python,
                "make-host": make_host_python,
                "build": build_all,
                "clean": clean_all,
                "setup-testbed": setup_testbed,
                "build-testbed": build_testbed,
                "test": run_testbed}

    try:
        result = dispatch[context.subcommand](context)
        if asyncio.iscoroutine(result):
            asyncio.run(result)
    except CalledProcessError as e:
        for stream_name in ["stdout", "stderr"]:
            content = getattr(e, stream_name)
            stream = getattr(sys, stream_name)
            if content:
                stream.write(content)
                if not content.endswith("\n"):
                    stream.write("\n")

        # Format the command so it can be copied into a shell.
        args_joined = " ".join(shlex.quote(str(arg)) for arg in e.cmd)
        sys.exit(
            f"Command '{args_joined}' returned exit status {e.returncode}"
        )


if __name__ == "__main__":
    main()
