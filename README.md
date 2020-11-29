# b6502
An series of emulators for MOS6502 based systems. 

## Usage
### Prerequisites
You will need the following software if you want to build this repository:
- C/C++ tooling (CMake, gcc/clang, etc.)
- SDL2

Example of how to install the above in Ubuntu:

```bash
sudo apt install -y build-essential libsdl2-dev clang
```

### Installing

You can use CMake to easily build the project.
```bash
cmake -Hstandalone -Bbuild
cmake --build build
```

You can start the emulator by running:
```bash
./build/b6502 --help
```

## Roadmap

See the [open issues](https://github.com/btorres510/b6502/issues) for a list of proposed features (and known issues).

## License

Distributed under the GNU GPL License. See `LICENSE` for more information.

## Acknowledgements

The template of this project was dervied from the following repositories: 

* [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter)
* [Modern C++ Template](https://github.com/filipdutescu/modern-cpp-template)

I primarily used Lars Melchior's excellent **ModernCppStarter**, and added features I liked from Filip Dutescu's **Modern C++ Template**. Any feature from **ModernCppStarter** will work here.