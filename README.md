#  Simulador de Sistema Operativo — C++

## 📋 Descripción General

Este proyecto se esta realizando para la entrega del proyecto final donde se implementa un **simulador de sistema operativo en C++**, diseñado para modelar los principios de planificación de CPU, manejo de memoria virtual y sincronización temporal.

Este READ.ME esta centrado en la entrega dos a la hora de realizar la entrega final se actualizará.

---

## 🧩 Fundamentos, Alcance y Arquitectura

El simulador cuenta con tres módulos principales:

| Módulo | Descripción |
|--------|-------------|
| **Planificador (Scheduler)** | Administra la CPU y selecciona qué proceso se ejecuta según la política elegida (Round Robin o SJF). |
| **Administrador de Memoria** | Gestiona las páginas y marcos de memoria mediante algoritmos FIFO y LRU. |
| **Interfaz de Línea de Comandos (CLI)** | Permite al usuario crear procesos, ejecutar ticks, cambiar políticas, y visualizar el estado del sistema. |

### 🎯 Objetivos de Diseño

- Simular la ejecución concurrente de procesos en un CPU virtual.  
- Implementar dos algoritmos de planificación (RR y SJF no expropiativo).  
- Implementar dos políticas de paginación (FIFO y LRU).  
- Permitir la observación y análisis de fallos de página según el tamaño de memoria.  
- Documentar el diseño, supuestos e invariantes de sincronización.

---

## ⚙️ Planificación de CPU

###  Round Robin (RR)
- Usa una cola circular de procesos READY.
- Quantum configurable (por defecto `2` ticks).
- Si un proceso agota su quantum sin finalizar, se mueve al final de la cola.

###  SJF No Expropiativo
- Selecciona el proceso con la menor ráfaga restante.
- No interrumpe el proceso actual hasta que termina.
- Reduce el tiempo promedio de espera respecto a RR.


---

## 🧠 Manejo de Memoria — Paginación

###  FIFO (First-In, First-Out)
- Cada página cargada entra a una cola.
- Cuando se llena la memoria, se reemplaza la más antigua.

###  LRU (Least Recently Used)
- Registra el último acceso de cada página.
- Se reemplaza la menos recientemente usada.
- Mejora el rendimiento gracias al principio de localidad temporal.

###  Métricas y Estadísticas
El sistema cuenta con contadores de:
- Accesos de página totales.
- Fallos de página (page faults).
- Reemplazos de marcos.

## 🚀 Cómo Ejecutarlo

###   Compilar el simulador principal

Abre una terminal en la carpeta donde guardes el  proyecto y ejecuta:

```bash
g++ -std=c++17 src/os_simulator_sjf_lru.cpp -o simulador
```
luego de hacer eso debes poner en la misma terminal 
```bash
./simulador
```
Ademas de esto nuestro simulador es user friendly o podras ver un comando de ayuda como digitalizando la palabra 
```bash
help
```
en la terminal del simulador.


## 👨‍💻 Desarrolladores
Este proyecto esta siendo desarrollado por estudiantes de ingenieria de sistemas de la Universidad EAFIT.

