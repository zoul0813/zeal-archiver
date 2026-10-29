# Library releases

Before committing and pushing library changes, rebuild the library with
`zde cmake build --clean-first` and include the updated tracked `lib/zar.lib`
in the commit.
Verify that the build succeeds and the archive exports the current public API.
