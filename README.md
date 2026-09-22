# Aetherburn

Aetherburn is an Unreal Engine game project.

## Repository setup

Git LFS is required because Unreal maps, assets, and other large binary source files are stored outside normal Git history.

1. Install [Git LFS](https://git-lfs.com/).
2. Run `git lfs install` once on your machine.
3. Clone the repository normally. Git LFS downloads the project assets automatically.
4. Right-click `Aetherburn.uproject` and generate project files when IDE files are needed.

Generated Unreal Engine folders and IDE solution files are intentionally not committed. Do not add `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`, or `.vs` to source control.
