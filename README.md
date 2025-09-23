# Standalone Beat Saber Streaming Plugin

Based on the [OBS Plugin Template](https://github.com/obsproject/obs-plugintemplate). See there for environment setup.

## Building

```sh
cmake --preset <preset>
cmake --build --preset <preset>
cmake --install build_x64 --config RelWithDebInfo --prefix <destination>
```

Possible values for preset are: `windows-x64`, `macos`, and `ubuntu-x86_64`. (I haven't actually built or tested anything other than Windows.) The destination prefix in the install command can be omitted and it should theoretically install it to your OBS directory.
