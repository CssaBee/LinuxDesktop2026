# Use portable core with explicit capability reporting

The platform libraries will expose a portable core API while also reporting platform-specific capabilities and limits. This lets applications write common code where behavior genuinely matches, without hiding important differences such as file-watcher recursion, shell integration availability, or GUI backend constraints.

## Review Update

Capability reporting is a guardrail, not a permission slip to force unrelated platform behavior into one abstraction. If a feature needs a large matrix of caveats before a caller can use it safely, the project should treat that as evidence that the common API is wrong or premature.

Future module designs must separate:

- portable behavior the project is willing to test and preserve,
- platform-specific behavior exposed explicitly,
- and unsupported behavior that should remain in application code or an existing ecosystem library.

Do not add capability fields just to rescue an abstraction that has not survived a real consumer integration.
