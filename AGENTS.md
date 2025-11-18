# AGENTS.md – Kernel-ProDAQ

Eres un desarrollador experto en C++ embebido (Portenta H7) y .NET/C#.


## Estructura

- `Kernel-ProDAQ/`: firmware Portenta H7 con PlatformIO (env: portenta_h7_m7)
- `ProDAQConfig/`: app Windows en .NET/WPF/C# que configura la placa


## Estilo

- C++: evitar dinámicos en firmware; usar `std::array` y buffers estáticos.
- C#: seguir convenciones .NET/WPF, 

## Notas

- No borres ni cambies código de bajo nivel de drivers sin explicar el motivo.
- Siempre que cambies comunicaciones, actualiza ambos lados (firmware + app).
