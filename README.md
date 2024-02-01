# Yarn Spinner for Unreal

<img src="https://yarnspinner.dev/img/YarnSpinnerLogo.png" alt="Yarn Spinner logo" width="100px;" align="right">

Welcome to **Yarn Spinner for Unreal!** Yarn Spinner is the friendly dialogue tool that makes it easy for writers to create content, and has powerful features for programmers.

> **Warning**
> This plugin is in an **extremely early alpha** state. Please see the list of [Caveats](#caveats). This release is intended to get early feedback from users. **Do not use this plugin in production.**

<img src=".github/assets/Epic_MegaGrants_Recipient_logo.png" alt="Yarn Spinner logo" width="100px;" align="right"> This project's development is supported by an Epic MegaGrant.

## Installation Instructions

1. If your project is already open in the Unreal Editor, close it.
2. Create a new folder called `Plugins` in the root of your project. (If you already have this folder, skip this step.)
3. Next, clone this repository and put it in the `Plugins` folder.
4. Finally, re-open your project. When Unreal Editor asks if you want to build the Yarn Spinner module, say yes.

## Caveats

There are several important Yarn Spinner features that are not yet present in the alpha release of Yarn Spinner for Unreal.

- **Windows & Mac support only.** This may extend to additional platforms in a future version.
- **Tested in Unreal Engine 4.27 only.** We haven't yet tested this in Unreal Engine 5.
- **No variables.** Storing and retrieving Yarn variables is not yet implemented.
- **No localisation support.** The Yarn Spinner importer does not currently populate string tables automatically.
- **No functions.** Functions cannot be called from inside Yarn scripts.
- **String-only command dispatch.** Dispatching commands to functions is not implemented; however, when a command is run, the Dialogue Runner emits an `OnRunCommand` event that contains the command name and an array of its parameters.
- **Single files only.** `.yarn` files are currently imported in isolation, and variables declared in one file will not be known in other files.

## Troubleshooting

### I get a "Plugin 'YarnSpinner' failed to load because module 'YarnSpinner' could not be found" message when I try to play a build of my game.

If your project was created as a Blueprint Project, then Unreal will not include plugins by default. Add a new empty C++ class (of any type), and rebuild your project.