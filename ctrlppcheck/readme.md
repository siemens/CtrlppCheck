# **Ctrlppcheck** 

Ctrlppcheck for ctrl/ctrl++ language

- all files ended by .ctl extentions shall be knonw as ctrl script (lib)
- known ctrl lang syntax
- known ctrl++ lang syntax
- ctrl-extention functions are configurable via config.xml files
- reg-exp for user defined checks via rule.xml files
- reg-exp to check code style

## Manual

- [ctrlPppCheckUsage](/docuSources/ctrlPppCheckUsage.md)
- [howTotestCtrlppCheck](/docuSources/howTotestCtrlppCheck.md)


## Compiling

on first usage

``` shell
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build .  --config Release
```

Debug:
```shell 
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build .  --config Debug
```

rebuild 

``` shell
cd build
cmake --build .  --config Release
```

## Error types

- Error: Code that will lead to errors. ()
- Warning: Code with which errors can arise in the future.
- Information: Information or code that would cause errors outside of CTRL but is ok in CTRL. (e.g.: Missing void, not initialized vars)
- Style: Code that is difficult to read and other style errors (e.g.: naming)


