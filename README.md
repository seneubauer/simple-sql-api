
# simple-sql-api

## Description

This library started out as a simple cross-platform ODBC wrapper written in C++20. It is supposed to serve as a performant and straightforward interface with SQL databases that abstracts away the C style API and provides convenience functionality. This library is not functional yet, so please don't use it.

## Install

The installation guide will go here. I encourage the use of CMake for building from source.

## To-Do

- Add a "last error" member to the `environment`, `database_connection`, and `statement` classes.
- Add an option to set `SQL_ATTR_ROW_ARRAY_SIZE` to control rowset binding.
