# Changelog

All notable changes to CardMesh will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- Project scaffold: directory layout, CMake build, `.gitignore`.
- `HttpClient`: libcurl-based HTTPS client with bearer auth and timeouts.
- `MeshMonitorClient`: typed wrapper over the MeshMonitor REST API v1 (sources, nodes, channels, messages, telemetry, traceroutes).
- Data models: `Source`, `Node`, `Channel`, `Message`, `Telemetry`, `Traceroute`.
- `Settings`: config load/save at `~/.config/cardmesh/config.json` with `0600` permissions.
- `Database`: SQLite-backed local cache and unread/favorites state.
- Unit tests for URL construction, JSON parsing, and settings persistence.
