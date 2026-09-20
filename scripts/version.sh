# Single source of truth for the release number: the project() line in
# CMakeLists.txt. Everything that stamps a version (deb, dmg, AppImage,
# Windows installer, the binary's --version) reads it from here, and the
# release tag is checked against it, so one number covers the lot.
#
# Usage:  . "$(dirname "$0")/version.sh"; VERSION="$(xquest_version)"

xquest_version() {
    local root
    root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    sed -n 's/^project(xquest VERSION \([0-9][0-9.]*\).*/\1/p' "$root/CMakeLists.txt"
}

# Resolve the version to build with. XQUEST_VERSION (the release tag, with or
# without its leading "v") may be passed in, but it has to agree with
# CMakeLists.txt: a tag that does not is a bump someone forgot, so stop rather
# than ship two different numbers.
xquest_resolve_version() {
    local project="$1" tag="${XQUEST_VERSION:-}"
    tag="${tag#v}"
    if [ -z "$tag" ]; then
        printf '%s\n' "$project"
        return 0
    fi
    if [ "$tag" != "$project" ]; then
        echo "version mismatch: tag says $tag, CMakeLists.txt says $project." >&2
        echo "Bump project(xquest VERSION ...) and retag." >&2
        return 1
    fi
    printf '%s\n' "$tag"
}
