.. Copyright © 2017 Lukas Rosenthaler, Andrea Bianco, Benjamin Geer,
   Tobias Schweizer, and Ivan Subotic.
   
   This file is part of Sipi.

   Sipi is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Sipi is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   Additional permission under GNU AGPL version 3 section 7:
   If you modify this Program, or any covered work, by linking or combining
   it with Kakadu (or a modified version of that library) or Adobe ICC Color
   Profiles (or a modified version of that library) or both, containing parts
   covered by the terms of the Kakadu Software Licence or Adobe Software Licence,
   or both, the licensors of this Program grant you additional permission
   to convey the resulting work.

   See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public
   License along with Sipi.  If not, see <http://www.gnu.org/licenses/>.

.. highlight:: none


###############
Developing Sipi
###############

.. contents:: :local:

************
Using an IDE
************

CLion
=====

If you are using the `CLion <https://www.jetbrains.com/clion/>`__ IDE,
put ``-j 1`` in Preferences -> Build, Execution, Deployment -> CMake ->
Build options, to prevent CMake from building with multiple processes.
Also, note that code introspection in the CLion editor may not work until
it has run CMake.

Code::Blocks
============

If you are using the `Code::Blocks`_ IDE, you can build a cdb project:

::

    cd build
    cmake .. -G "CodeBlocks - Unix Makefiles"

*************
Writing Tests
*************

We use two test frameworks. We use googletest_ for unit test and pytest_ for
end-to-end tests.

Unit Tests
===========

TBA


End-to-End Tests
=================

To add end-to-end tests, add a Python class in a file
whose name begins with ``test``, in the ``test`` directory. The class's
methods, whose names must also begin with ``test``, should use the ``manager``
fixture defined in ``test/conftest.py``, which handles starting and stopping a
Sipi server, and provides other functionality useful in tests. See the
existing ``test/test_*.py`` files for examples.

To facilitate testing client HTTP connections in Lua scripts, the ``manager``
fixture also starts and stops an ``nginx`` instance, which can be used to
simulate an authorization server. For example, the provided ``nginx``
configuration file, ``test/nginx/nginx.conf``, allows ``nginx`` to act as a
dummy Knora_ API server for permission checking: its ``/v1/files`` route
returns a static JSON file that always grants permission to view the requested
file.

*********************
Commit Message Schema
*********************

When writing commit messages, we stick to this schema:

::

    type (scope): subject
    body

Types:

- feature (new feature for the user)
- fix (bug fix for the user)
- docs (changes to the documentation)
- style (formatting, etc)
- refactor (refactoring production code, e.g. renaming a variable)
- test (adding missing tests, refactoring tests)
- build (changes to CMake configuration, etc.)
- enhancement (residual category)

Example:

::

    feature (HTTP server): support more authentication methods

.. _googletest: https://github.com/google/googletest
.. _pytest: http://doc.pytest.org/en/latest/
.. _Knora: http://www.knora.org/
