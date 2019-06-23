# Cflat
### Embeddable lightweight scripting language with C++ syntax

Embeddable scripting languages provide the possibility of implementing and tweaking features during the software development process without the need of recompiling or restarting after each code change. This is particularly useful when working with large codebases which take long to compile.

Unfortunately, such an advantage usually implies some additional runtime costs, like a slow execution time (at least in comparison with what would be an equivalent compiled version of the same code written in C++) and a big amount of heap allocations, which might be considerable in performance-critical software like videogames.

Cflat is an embeddable scripting language whose syntax is 100% compatible with C++, what means that all the scripts written for Cflat can be actually compiled in release builds along with the rest of the code.

**In this case, compile means compile** - the scripts are not compiled into some kind of bytecode, though: they are compiled into machine code for the specific platform you are targeting, just like the rest of the C++ code from the project.

Both software engineers and other members of the team can benefit from Cflat. Regarding the second group, one might say that C++ is not the best choice for developers who are not software engineers or programmers, and C++ is indeed not as friendly as other scripting languages, but the truth is that high-level C++ code, which is the kind of code you usually write in scripts, does not look that different from other widely used languages like C# or Java(Script).

Cflat does not intend to be a fully featured C++ interpreter, but rather a lightweight scripting language whose syntax is 100% compatible with C++. This means that:
- Only a (small) subset of features from C++ are available in Cflat
- Everything from Cflat can be compiled with any C++11 compliant compiler

In case you are looking for a proper C++ interpreter, you might want to take a look at the following alternatives:
- [cling](https://github.com/root-project/cling) (extremely heavy, depends on clang and llvm)
- [Ch](https://www.softintegration.com/) (commercial)
- [CINT](http://www.hanno.jp/gotom/Cint.html)


## FAQ

Is any C++ code compatible with Cflat?
- *No. Cflat supports only a rather small subset of features from the C++11 standard.*


Is any Cflat code compatible with C++?
- *Yes. Cflat code can be compiled with any existing C++11 compiler.*


Is it in Cflat's roadmap to eventually support all the features from C++?
- *No. The idea is to keep it simple and lightweight. Although it is indeed planned to progressively add features to it and make it more powerful, Cflat will never provide all the features any C++ standard provides.*


Is Cflat going to provide any extra features outside from C++ in the future?
- *No. Cflat code shall always be 100% compatible with C++.*


Does Cflat require any external dependencies?
- *No. Only standard C++11 features and STL containers are required.*


Is Cflat cross-platform?
- *Yes, as cross-platform as any C++ code can be. If there is a C++11 compiler for the platform you are targeting, Cflat can be used on it.*

