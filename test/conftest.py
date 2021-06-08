# python-exec -- test fixtures
# (c) 2021 Michał Górny
# Licensed under the terms of the 2-clause BSD license.

import os
import pathlib
import shutil

import pytest


try:
    PYTHON_IMPLS = [x for x in os.environ['PYTHON_IMPLS'].split()
                    if shutil.which(x) is not None]
except KeyError:
    raise RuntimeError('Please set PYTHON_IMPLS to list of supported impls')


@pytest.fixture
def test_dir():
    td = pathlib.Path('test/data')
    shutil.rmtree(td, ignore_errors=True)
    os.makedirs(td, exist_ok=True)
    yield td


@pytest.fixture(scope='session', params=PYTHON_IMPLS)
def every_python(request):
    """Return all Python interpreters that are supported and installed."""
    yield request.param



@pytest.fixture(scope='session', params=PYTHON_IMPLS[:-1])
def nonbest_python(request):
    """Return all Python interpreters except the best one."""
    yield request.param


@pytest.fixture(scope='session', params=[os.symlink, shutil.copy])
def copy_method(request):
    """Parametrize on symlinking and copying."""
    yield request.param
