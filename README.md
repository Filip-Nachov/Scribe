
# Scribe

A simple terminal text editor

[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)
![GitHub commit activity](https://img.shields.io/github/commit-activity/t/Filip-Nachov/Scribe)
![GitHub Created At](https://img.shields.io/github/created-at/Filip-Nachov/Scribe)
![Static Badge](https://img.shields.io/badge/Scribe-1.0-blue)


## Features

- Syntax Highlighting for different filetypes like:
    - C
    - C++
    - Python
    - Java (currently being added)
    - Golang (currently being added)
- Search function
- Movemening the cursor with the keyboard
- Keyboard shortcuts
- basic file operations (saving, creating, opening, etc)



## Installation

### Prerequirments
<b> [git](https://git-scm.com/downloads) <b> <br>
<b> [make](https://www.incredibuild.com/integrations/gnu-make) <b> <br>
<b> [gcc](https://www.incredibuild.com/integrations/gcc) <b>

### Linux/Macos

In order to install scribe with Linux/Macos you can just clone the repo and use the make file in it   

```bash
git clone https://github.com/Filip-Nachov/Scribe.git
cd Scribe
make
sudo make install
```
and to test and make sure everything is done correctly, run the command 

```
scribe
```

### Windows
The best way to do this now is with a [wsl](https://learn.microsoft.com/en-us/windows/wsl/install) and use the Linux/Macos guide


    
## Usage

Here will be shown the basic file operations, movement, writing (insert mode) 

### file operations
To open Scribe use:
```bash
scribe
```
To open a file using Scribe:
```bash
scribe filename.ext
```

To quit without saving use Ctrl-Q and to save use Ctrl-S (if saving for the first time you will have to type in a name) to Search trought the file use "/" (if there are multiple results you can move around them with arrow up and arrow down)
### Movement
To move around in it use h, j, k, l
or arrow keys if you prefer them. To move at the begining of a word use A or Home Key and for the End of the word use D or END key. To move at the end of a screen use M or PAGE_DOWN and PAGE UP or N to move at the begining of the screen!

### Writing
To write like in many other text editors you have to use i to go to insert mode and type while in there you can move around with the arrow keys and click escape to quit aka go back to normal mode. The file operations dont work while in insert mode
## Features in development

Features that are supposed to come bofore 2.0 or at 2.0 depending on the feature is:

- file tree
- themes
- configuration
- file searching
- theme selector 
- Scribe coming to windows with other than WSL 
- Scribe coming to packages from linux distros 
## Contributing

Contributions are always welcome!🤗

To contribute you can either fork the project, code a feature or fix an issue and make a pull request or you can create a github issue either reporting a bug/error or reccomending a feature!

Whatever you do please make sure to be very descriprive so others can understand otherwise the code or github issue might be rejected or closed.


## License

this project is under the [GNU GPL](LICENSE) License


## Authors

- [Filip Nachov](https://www.github.com/Filip-Nachov)


Current version: 1.0
