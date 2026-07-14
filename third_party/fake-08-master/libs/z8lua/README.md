# z8lua

This is a fork of Lua that implements the [PICO-8](https://www.lexaloffle.com/pico-8.php)
dialect and adds useful features for emulator implementations.

### Branches

The main `zepto8` branch is a composite branch built from several feature branches. Please
try to submit patches and PRs against the corresponding feature branch instead of `zepto8`.

There are three main feature branches:

 - `pico8`: this is a “clean” branch that only implements the PICO-8 syntax and type system,
   with no extra fancy features; a good start if you’re writing your own emulator.
 - `eris`: a branch imported from the [eris persistence patch](https://github.com/fnuecke/eris)
   with bug fixes and improvements ([my pull requests have received little attention so
   far](https://github.com/fnuecke/eris/pulls)); this library provides serialisable snapshots
   of the Lua state (quite useful for emulators).
 - `oua`: experimental branch allowing to switch on-the-fly to zero-based indices in Lua, using
   the `base(0)` function and back with `base(1)`. I call this the **Oua** language.

## PICO-8 features

 - short `if` syntax (on one line)
 - short `print` syntax (`?"hello"`)
 - compound assignment operators: `+=` `/=` etc.
 - C style not equal operator: `!=`
 - C++ style comments with `//`
 - fixed-point arithmetic with overflows, infinity etc.
 - the PICO-8 math library (`shr`, `atan2`, `flr` etc.)
 - binary literals: `0b1001001.10010`
 - works in Windows, Linux, OS X, and many embedded systems

### Limitations

 - Lua functions that rely on the PICO-8 state, particularly the VM memory, are beyond the scope
   of this software; for a more complete PICO-8 implementation, see the [zepto8
   emulator](https://github.com/samhocevar/zepto8) which is based on z8lua. The only exceptions
   are the `@`, `%` and `$` operators (see next section).
 - the `^` (power) operator is implemented using floating point, which is inelegant and a
   potential performance issue, and which also means the results are not bit-by-bit equivalent to
   the original PICO-8.

## API extensions

```c
LUA_API void lua_setpico8memory (lua_State *L, unsigned char const *p);
```

Provide the Lua VM with a 64-KiB address space for use with the `@`, `%` and `$` operators
(shorthands for `peek`, `peek2`, and `peek4`). Otherwise these operators will always return 0.
