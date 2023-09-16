import os
import os.path
import subprocess
import sys
import sysconfig
import tempfile
from importlib import resources


__all__ = ["version", "bootstrap"]
_PIP_VERSION = "23.3.2"

# Directory of system wheel packages. Some Linux distribution packaging
# policies recommend against bundling dependencies. For example, Fedora
# installs wheel packages in the /usr/share/python-wheels/ directory and don't
# install the ensurepip._bundled package.
_WHEEL_PKG_DIR = sysconfig.get_config_var('WHEEL_PKG_DIR')


def _find_wheel_pkg_dir_pip():
    if _WHEEL_PKG_DIR is None:
        return None
    try:
        filenames = os.listdir(_WHEEL_PKG_DIR)
    except OSError:
        # Ignore: path doesn't exist or permission error
        return None
    # Make the code deterministic if a directory contains multiple wheel files
    # of the same package, but don't attempt to implement correct version
    # comparison since this case should not happen.
    filenames = sorted(filenames, reverse=True)
    for filename in filenames:
        # filename is like 'pip-21.2.4-py3-none-any.whl'
        if not filename.startswith("pip-") or not filename.endswith(".whl"):
            continue

        # Extract '21.2.4' from 'pip-21.2.4-py3-none-any.whl'
        version = filename.removeprefix("pip-").partition("-")[0]
        return {"version": version, "filename": filename, "bundled": False}

    return None


def _get_pip_info():
    # Prefer pip from the wheel package directory, if present.
    if (pip_info := _find_wheel_pkg_dir_pip()) is not None:
        return pip_info
    filename = f"pip-{_PIP_VERSION}-py3-none-any.whl"
    return {"version": _PIP_VERSION, "filename": filename, "bundled": True}


def _run_pip(args, additional_paths=None):
    # Run the bootstrapping in a subprocess to avoid leaking any state that happens
    # after pip has executed. Particularly, this avoids the case when pip holds onto
    # the files in *additional_paths*, preventing us to remove them at the end of the
    # invocation.
    code = f"""
import runpy
import sys
sys.path = {additional_paths or []} + sys.path
sys.argv[1:] = {args}
runpy.run_module("pip", run_name="__main__", alter_sys=True)
"""

    cmd = [
        sys.executable,
        '-W',
        'ignore::DeprecationWarning',
        '-c',
        code,
    ]
    if sys.flags.isolated:
        # run code in isolated mode if currently running isolated
        cmd.insert(1, '-I')
    return subprocess.run(cmd, check=True).returncode


def version():
    """
    Returns a string specifying the bundled version of pip.
    """
    return _get_pip_info()["version"]


def _disable_pip_configuration_settings():
    # We deliberately ignore all pip environment variables
    # when invoking pip
    # See http://bugs.python.org/issue19734 for details
    keys_to_remove = [k for k in os.environ if k.startswith("PIP_")]
    for k in keys_to_remove:
        del os.environ[k]
    # We also ignore the settings in the default pip configuration file
    # See http://bugs.python.org/issue20053 for details
    os.environ['PIP_CONFIG_FILE'] = os.devnull


def bootstrap(*, root=None, upgrade=False, user=False,
              altinstall=False, default_pip=False,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory).

    Note that calling this function will alter both sys.path and os.environ.
    """
    # Discard the return value
    _bootstrap(root=root, upgrade=upgrade, user=user,
               altinstall=altinstall, default_pip=default_pip,
               verbosity=verbosity)


def _bootstrap(*, root=None, upgrade=False, user=False,
              altinstall=False, default_pip=False,
              verbosity=0):
    """
    Bootstrap pip into the current Python installation (or the given root
    directory). Returns pip command status code.

    Note that calling this function will alter both sys.path and os.environ.
    """
    if altinstall and default_pip:
        raise ValueError("Cannot use altinstall and default_pip together")

    sys.audit("ensurepip.bootstrap", root)

    _disable_pip_configuration_settings()

    # By default, installing pip installs all of the
    # following scripts (X.Y == running Python version):
    #
    #   pip, pipX, pipX.Y
    #
    # pip 1.5+ allows ensurepip to request that some of those be left out
    if altinstall:
        # omit pip, pipX
        os.environ["ENSUREPIP_OPTIONS"] = "altinstall"
    elif not default_pip:
        # omit pip
        os.environ["ENSUREPIP_OPTIONS"] = "install"

    with tempfile.TemporaryDirectory() as tmpdir:
        # Put our bundled wheels into a temporary directory and construct the
        # additional paths that need added to sys.path
        package = _get_pip_info()
        wheel_name = package["filename"]
        if package["bundled"]:
            # Use bundled wheel package
            wheel_path = resources.files("ensurepip") / "_bundled" / wheel_name
            whl = wheel_path.read_bytes()
        else:
            # Use the wheel package directory
            with open(os.path.join(_WHEEL_PKG_DIR, wheel_name), "rb") as fp:
                whl = fp.read()

        filename = os.path.join(tmpdir, wheel_name)
        with open(filename, "wb") as fp:
            fp.write(whl)

        # Construct the arguments to be passed to the pip command
        args = ["install", "--no-cache-dir", "--no-index", "--find-links", tmpdir]
        if root:
            args += ["--root", root]
        if upgrade:
            args += ["--upgrade"]
        if user:
            args += ["--user"]
        if verbosity:
            args += ["-" + "v" * verbosity]

        return _run_pip([*args, "pip"], [filename])


def _uninstall_helper(*, verbosity=0):
    """Helper to support a clean default uninstall process on Windows

    Note that calling this function may alter os.environ.
    """
    # Nothing to do if pip was never installed, or has been removed
    try:
        import pip
    except ImportError:
        return

    # If the installed pip version doesn't match the available one,
    # leave it alone
    available_version = version()
    if pip.__version__ != available_version:
        print(f"ensurepip will only uninstall a matching version "
              f"({pip.__version__!r} installed, "
              f"{available_version!r} available)",
              file=sys.stderr)
        return

    _disable_pip_configuration_settings()

    # Construct the arguments to be passed to the pip command
    args = ["uninstall", "-y", "--disable-pip-version-check"]
    if verbosity:
        args += ["-" + "v" * verbosity]

    return _run_pip([*args, "pip"])


def _main(argv=None):
    import argparse
    parser = argparse.ArgumentParser(prog="python -m ensurepip")
    parser.add_argument(
        "--version",
        action="version",
        version="pip {}".format(version()),
        help="Show the version of pip that is bundled with this Python.",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="count",
        default=0,
        dest="verbosity",
        help=("Give more output. Option is additive, and can be used up to 3 "
              "times."),
    )
    parser.add_argument(
        "-U", "--upgrade",
        action="store_true",
        default=False,
        help="Upgrade pip and dependencies, even if already installed.",
    )
    parser.add_argument(
        "--user",
        action="store_true",
        default=False,
        help="Install using the user scheme.",
    )
    parser.add_argument(
        "--root",
        default=None,
        help="Install everything relative to this alternate root directory.",
    )
    parser.add_argument(
        "--altinstall",
        action="store_true",
        default=False,
        help=("Make an alternate install, installing only the X.Y versioned "
              "scripts (Default: pipX, pipX.Y)."),
    )
    parser.add_argument(
        "--default-pip",
        action="store_true",
        default=False,
        help=("Make a default pip install, installing the unqualified pip "
              "in addition to the versioned scripts."),
    )

    args = parser.parse_args(argv)

    return _bootstrap(
        root=args.root,
        upgrade=args.upgrade,
        user=args.user,
        verbosity=args.verbosity,
        altinstall=args.altinstall,
        default_pip=args.default_pip,
    )
