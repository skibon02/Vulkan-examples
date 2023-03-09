## VulkanTriangle
Этот код можно использовать для того, чтобы убедиться, что вулкан и библиотеки установленны корректно

В качестве сред разработки можно использовать CLion либо VS Code с плагином CMake.

Сборка проекта с консоли:
```bash
cd VulkanTriangle
mkdir build && cd build
cmake ..
make
```

## Сборка на Linux:
1) Скачать glslc и добавить в PATH: https://askubuntu.com/questions/1252585/how-to-install-glslc-on-ubuntu-20-04
2) поставить библиотеку glfw3: sudo apt-get install libglfw3-dev (для Debian-based)
