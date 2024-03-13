# Yarn Spinner for Unreal

<img src="https://downloads.yarnspinner.dev/get/YarnSpinnerLogo.png" alt="Yarn Spinner logo" width="100px;" align="right">

Welcome to **Yarn Spinner for Unreal!** Yarn Spinner is the friendly dialogue tool that makes it easy for writers to create content, and has powerful features for programmers.

Yarn Spinner for Unity is designed for **Unreal Engine 5.3 and above.** It may work on earlier versions, but our current focus is on 5.3.

> [!IMPORTANT]
> This plugin is a **beta**. Please see the list of [Known Issues](#known-issues). This release is intended to get early feedback from users. **Do not use this plugin in production.**

<img src=".github/assets/Epic_MegaGrants_Recipient_logo.png" alt="Yarn Spinner logo" width="50px;" align="right"> This project's development is supported by an Epic MegaGrant.

## Tutorial

To get started with Yarn Spinner for Unreal, we have an [end-to-end tutorial](https://docs.yarnspinner.dev/using-yarnspinner-with-unreal/tutorial) available, starting from a blank project and ending with voice-acted interactive conversation!

## Installation Instructions

1. If your project is already open in the Unreal Editor, close it.
2. Create a new folder called `Plugins` in the root of your project. (If you already have this folder, skip this step.)
3. Next, clone this repository and put it in the `Plugins` folder.
4. Finally, re-open your project. When Unreal Editor asks if you want to build the Yarn Spinner module, say yes.

## Known Issues

Yarn Spinner for Unreal is currently at a beta stage of development, and there are a number of important caveats to know.

- **Windows support only.** This may extend to additional platforms in a future version.
- **In-memory variable storage only.** Variables can be stored and retrieved, but they are not currently stored on disk.
- **Limited function support.**
  - The following functions are supported: `visited`, `visited_count`, `string`, `number`, `bool`.
- **String-only command dispatch.** Dispatching commands to functions is not implemented; however, when a command is run, the Dialogue Runner emits an `OnRunCommand` event that contains the command name and an array of its parameters.
- **Yarn Project importing takes longer than desired.** When you import a Yarn Project asset, it may take several seconds for the process to complete, during which time the Editor will not be responsive. 
- **String tables may incorrectly cache in the Editor.** When you import a Yarn Project, the string table will be populated with its contents. If you make changes to the Yarn files and re-import the Yarn Project, the string table contents will update, but the editor may still hold the cached values from the earlier version, resulting in incorrect lines being displayed. As a workaround for this issue, play the game in Standalone mode. Quitting and relaunching the Editor will also reset this cache.

## Troubleshooting

### I get a "Plugin 'YarnSpinner' failed to load because module 'YarnSpinner' could not be found" message when I try to play a build of my game.

If your project was created as a Blueprint Project, then Unreal will not include plugins by default. Add a new empty C++ class (of any type), and rebuild your project.

## Credits

Yarn Spinner for Unreal was developed by Yarn Spinner Pty Ltd, with enormous amounts of help from Ben Phelan.