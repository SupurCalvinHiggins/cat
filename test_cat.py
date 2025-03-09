import pytest
import subprocess
from pathlib import Path
from typing import Callable


@pytest.fixture(scope="session")
def session_tmp_path(tmp_path_factory) -> Path:
    session_tmp = tmp_path_factory.mktemp("session_tmp")
    return session_tmp


@pytest.fixture(scope="session")
def compile_cmake(session_tmp_path: Path) -> Callable[[Path, str], Path]:
    def _compile_cmake(cmake_path: Path, executable_name: str) -> Path:
        cmake_path = cmake_path.resolve()

        build_path = session_tmp_path / f"build-{hash(executable_name)}"
        executable_path = build_path / executable_name

        build_path = build_path.resolve()
        executable_path = executable_path.resolve()

        if executable_path.exists():
            return executable_path

        build_path.mkdir(exist_ok=True)
        subprocess.run(
            [
                "cmake",
                "-G",
                "Unix Makefiles",
                "-B",
                build_path.as_posix(),
                "-S",
                cmake_path.as_posix(),
            ]
        )
        subprocess.run(["make", "-C", build_path.as_posix()])

        return executable_path

    return _compile_cmake


@pytest.fixture(scope="session")
def cat_path(compile_cmake: Callable[[Path, str], Path]) -> Path:
    return compile_cmake(Path("."), "cat")


@pytest.fixture
def create_file(tmp_path: Path) -> Callable[[str, bytes], Path]:
    def _create_file(file_name: str, content: bytes) -> Path:
        file_path = tmp_path / file_name
        file_path.write_bytes(content)
        return file_path

    return _create_file


@pytest.mark.parametrize(
    ["args", "files", "expected_stdout", "expected_returncode"],
    [
        [[], {"-": b""}, b"", 0],
        [[], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["-"], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["-", "-"], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["a.txt"], {"-": b"no", "a.txt": b"hello\nworld"}, b"hello\nworld", 0],
        [
            ["a.txt", "-"],
            {"-": b"yes", "a.txt": b"hello\nworld"},
            b"hello\nworldyes",
            0,
        ],
        [
            ["-", "a.txt"],
            {"-": b"yes", "a.txt": b"hello\nworld"},
            b"yeshello\nworld",
            0,
        ],
        [
            ["a.txt", "b.bin"],
            {"-": b"no", "a.txt": b"hello\nworld", "b.bin": b"data"},
            b"hello\nworlddata",
            0,
        ],
        [
            ["a.txt", "a.txt"],
            {"-": b"no", "a.txt": b"hello\nworld"},
            b"hello\nworldhello\nworld",
            0,
        ],
        [["a.txt"], {"-": b"no", "a.txt": b"a" * 1_000_000}, b"a" * 1_000_000, 0],
        [["-u"], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["-u", "-u"], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["-u", "-"], {"-": b"hello\nworld"}, b"hello\nworld", 0],
        [["-u", "a.txt"], {"-": b"no", "a.txt": b"hello\nworld"}, b"hello\nworld", 0],
        [["-f"], {"-": b""}, b"", 1],
        [["-u", "-f"], {"-": b""}, b"", 1],
        [["-f", "-u"], {"-": b""}, b"", 1],
        [["dne.txt"], {"-": b""}, b"", 1],
        [["-", "dne.txt"], {"-": b""}, b"", 1],
        [["a.txt", "dne.txt"], {"-": b"", "a.txt": b""}, b"", 1],
    ],
    ids=[
        "none_no_stdin",
        "none",
        "dash",
        "dash_dash",
        "file",
        "file_dash",
        "dash_file",
        "file0_file1",
        "file0_file0",
        "largefile",
        "u_none",
        "u_u_none",
        "u_dash",
        "u_file",
        "badflag",
        "u_badflag",
        "badflag_u",
        "dne",
        "dash_dne",
        "file_dne",
    ],
)
def test_cat(
    cat_path: Path,
    create_file: Callable[[str, bytes], Path],
    args: list[str],
    files: dict[str, bytes],
    expected_stdout: bytes,
    expected_returncode: int,
) -> None:
    input = files.pop("-", b"")

    file_name_to_path = {
        file_name: create_file(file_name, content)
        for file_name, content in files.items()
    }

    for i in range(len(args)):
        if args[i] in file_name_to_path:
            args[i] = file_name_to_path[args[i]].as_posix()

    proc = subprocess.run(
        [cat_path.as_posix(), *args],
        input=input,
        capture_output=True,
        timeout=5,
    )

    assert proc.returncode == expected_returncode
    if expected_returncode == 0:
        assert proc.stdout == expected_stdout
