# Security Policy – CardMesh

CardMesh is a pocket client for MeshMonitor, running on the M5Stack CardputerZero. It authenticates to a user-configured MeshMonitor server over HTTPS using an API token, and may display private mesh channel messages, direct messages, and node telemetry. Because it handles credentials and potentially sensitive field/deployment data, security must be taken seriously.

---

## Reporting a Vulnerability

If you discover a security vulnerability:

- Do NOT open a public issue.
- Email the maintainer directly or use GitHub’s Private Vulnerability Reporting feature.
- Include:
  - A clear description of the issue
  - Steps to reproduce
  - Affected version(s)
  - Any logs or screenshots (sanitized — remove tokens, server addresses, and message content)

Please allow reasonable time for investigation and patching before public disclosure.

Responsible disclosure is required.

---

## API Tokens & Credentials

CardMesh authenticates to MeshMonitor using a Bearer API token (see `Authorization: Bearer <API_TOKEN>` in the README).

Requirements:

- API tokens must never be logged, printed, or included in crash reports.
- API tokens must never be committed to this repository.
- Configuration is stored at `~/.config/cardmesh/config.json` with `0600` permissions (see `Settings::save` in `src/storage/Settings.cpp`) — do not weaken this permission model.
- Token entry in the UI should be obscured (masked input), consistent with the "First Launch" setup screen in the README.

---

## Network & Transport Security

CardMesh communicates with a single, user-configured MeshMonitor server.

- HTTPS with TLS certificate verification is enabled by default (`HttpClient::setVerifyTls`) and must not be disabled except for explicit, user-opted-in development/testing against a local server.
- CardMesh does not run its own network-facing server or listener; it is an outbound-only REST client.
- CardMesh does not implement analytics, advertising, or any third-party telemetry. All traffic goes directly to the configured MeshMonitor instance.

---

## Message & Mesh Data Privacy

Channel messages, direct messages, and node telemetry retrieved from MeshMonitor may be sensitive (e.g. field/deployment communications).

- Do not add any feature that relays CardMesh data to a third-party service.
- Local caches (`cardmesh.db`, SQLite) contain only CardMesh-specific state (read state, favorites, cached API responses) and must not be committed or shared.
- Treat mesh traffic as potentially unencrypted at the radio layer; CardMesh's responsibility is limited to the MeshMonitor↔CardMesh transport, not the underlying RF network.

---

## Secrets & Configuration

If CardMesh gains support for additional credentials (e.g. alternate auth schemes, webhook integrations):

- They must be stored in `~/.config/cardmesh/config.json` (or an equivalent restricted-permission file), not hardcoded.
- They must not be committed to Git.
- They must not appear in logs (`~/.local/state/cardmesh/cardmesh.log`).

---

## Secure Development Practices

CardMesh development guidelines:

- No hardcoded credentials or debug backdoors.
- Validate and gracefully handle malformed or unexpected JSON from MeshMonitor — never assume the response shape (see `src/models/JsonUtil.h`).
- Avoid arbitrary shell execution.
- Sanitize any MeshMonitor-provided text (node names, message text) before rendering, since it originates from other mesh participants and should be treated as untrusted input.

Pull Requests affecting `HttpClient`, `MeshMonitorClient`, or `Settings` will receive additional review.

---

## Supported Versions

Security updates are applied to:

- The latest tagged release
- The most recent minor version branch (if applicable)

Older versions may not receive patches.
