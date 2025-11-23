# Лабораторная работа 2

## Зависимости

Компилятор, совместимый со стандартом C++11; CMake

```bash
pacman -Syu gcc cmake
```

## Компиляция

```bash
mkdir -p build
cd build
cmake ..
make
```

## Тестирование

```bash
cd build
ctest
```

![МИФИ](./contrib/mephi.png)
