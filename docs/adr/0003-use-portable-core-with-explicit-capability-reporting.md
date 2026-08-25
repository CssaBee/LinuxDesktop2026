# Use portable core with explicit capability reporting

The platform libraries will expose a portable core API while also reporting platform-specific capabilities and limits. This lets applications write common code where behavior genuinely matches, without hiding important differences such as file-watcher recursion, shell integration availability, or GUI backend constraints.
