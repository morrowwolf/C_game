Old version of attempted game engine coded in purely C with the standard library, Windows win32 header, and DirectX headers.

New version utilizes GLFW/Vulkan and Clang's C23 implementation and is closed as of now. No more work will be done on this repo.

A note on memory ordering: MSVC and Windows utilize volatiles and memory ordering under the assumption of both aligned memory and x86. This assumption allows them to "guarantee" that volatile reads are "acquires" and correctly ordered. A lot of this code is reliant on this assumption and other windows specific assumptions which is part of why I ended up changing directions entirely and starting the new version.
