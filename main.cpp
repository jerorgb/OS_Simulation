// os_simulator_sjf_lru.cpp
// Simulador SO con CLI, scheduler (RR y SJF no-expropiativo) y paginacion (FIFO/LRU global).
// Compilar: g++ -std=c++17 os_simulator_sjf_lru.cpp -o os_simulator
// Ejecutar: ./os_simulator
#include <bits/stdc++.h>
using namespace std;


// Tipos y utilidades

enum class Estado { NEW, READY, RUNNING, BLOCKED, TERMINATED };
string estado_to_str(Estado e) {
    switch (e) {
        case Estado::NEW: return "NEW";
        case Estado::READY: return "READY";
        case Estado::RUNNING: return "RUNNING";
        case Estado::BLOCKED: return "BLOCKED";
        case Estado::TERMINATED: return "TERMINATED";
    }
    return "?";
}

static std::mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());


// PCB (Bloque de Control de Proceso)

struct PCB {
    int pid;
    Estado estado;
    int rafaga_restante;   // tiempo CPU restante
    int rafaga_total;
    int llegada_tick;
    int inicio_tick;       // primer tick que corrió
    int fin_tick;          // tick de finalización
    int espera_acumulada;
    // Memoria virtual: páginas del proceso (0..npages-1)
    int npages;
    vector<int> trace;     // traza opcional de páginas a acceder en cada ejecución
    int trace_pos;

    // estadísticas de paginación
    int page_faults;

    PCB(int _pid=0, int burst=0, int now=0, int pages=4)
        : pid(_pid), estado(Estado::NEW), rafaga_restante(burst),
          rafaga_total(burst), llegada_tick(now), inicio_tick(-1), fin_tick(-1),
          espera_acumulada(0), npages(pages), trace_pos(0), page_faults(0) {}
};


// Frame y administrador de memoria

struct Frame {
    int fid;        // frame id
    int pid;        // id del proceso dueño, -1 si está libre
    int page;       // número de pagina
    long long loaded_at_tick; // para FIFO
    long long last_access_tick; // para LRU
    Frame(int id=0): fid(id), pid(-1), page(-1), loaded_at_tick(-1), last_access_tick(-1) {}
};

enum class ReplPolicy { FIFO, LRU };

// Administrador de memoria: conjunto de frames global, reemplazo global
class MemoryManager {
private:
    vector<Frame> frames;
    ReplPolicy policy;
    long long tick_counter = 0;
    // Para FIFO mantenemos una cola de IDs de frames en orden de carga
    deque<int> fifo_queue;

    // estadísticas
    int total_page_faults = 0;
    int total_replacements = 0;

public:
    MemoryManager(int nframes=8, ReplPolicy p=ReplPolicy::FIFO) {
        frames.reserve(nframes);
        for (int i = 0; i < nframes; ++i) frames.emplace_back(i);
        policy = p;
    }

    void set_policy(ReplPolicy p) {
        policy = p;
        // reinicia la cola FIFO según los frames cargados
        fifo_queue.clear();
        for (auto &f : frames) if (f.pid != -1) fifo_queue.push_back(f.fid);
    }

    ReplPolicy get_policy() const { return policy; }

    int num_frames() const { return (int)frames.size(); }

    void advance_tick() { tick_counter++; }

  // Verifica si (pid,page) está en memoria; si sí, actualiza LRU y retorna true
    bool is_resident_and_touch(int pid, int page) {
        for (auto &f : frames) {
            if (f.pid == pid && f.page == page) {
                f.last_access_tick = tick_counter;
                return true;
            }
        }
        return false;
    }

    // Carga (pid,page) en memoria, posiblemente reemplazando otro frame
    int load_page(int pid, int page) {
        total_page_faults++;
        // busca frame libre
        for (auto &f : frames) {
            if (f.pid == -1) {
                f.pid = pid; f.page = page;
                f.loaded_at_tick = tick_counter;
                f.last_access_tick = tick_counter;
                if (policy == ReplPolicy::FIFO) fifo_queue.push_back(f.fid);
                return f.fid;
            }
        }
        // No hay frame libre, entonces se reemplazar
        int victim_fid = choose_victim();
        // realizar reemplazo
        Frame &vf = frames[victim_fid];
        vf.pid = pid;
        vf.page = page;
        vf.loaded_at_tick = tick_counter;
        vf.last_access_tick = tick_counter;
        total_replacements++;
        if (policy == ReplPolicy::FIFO) {
           // rotar la cola FIFO: eliminar víctima y agregar nuevo
            auto it = find(fifo_queue.begin(), fifo_queue.end(), victim_fid);
            if (it != fifo_queue.end()) fifo_queue.erase(it);
            fifo_queue.push_back(victim_fid);
        }
        return victim_fid;
    }
    // Selección de víctima para reemplazo
    int choose_victim() {
        if (policy == ReplPolicy::FIFO) {
    // La víctima está al frente de la cola FIFO
            if (fifo_queue.empty()) {
                // fallback: encuentra el más antiguo por loaded_at_tick
                long long minload = LLONG_MAX; int fid=0;
                for (auto &f: frames) if (f.loaded_at_tick < minload) { minload=f.loaded_at_tick; fid=f.fid; }
                return fid;
            } else {
                int fid = fifo_queue.front();
                fifo_queue.pop_front();
                return fid;
            }
        } else { // LRU
            long long min_last = LLONG_MAX; int fid = 0;
            for (auto &f : frames) {
                if (f.last_access_tick < min_last) {
                    min_last = f.last_access_tick;
                    fid = f.fid;
                }
            }
            return fid;
        }
    }

    // API: acceso a página, retorna par (tenía_página(bool), id_frame)
    pair<bool,int> access_page(int pid, int page) {
    // Incrementar el contador de ticks para el contexto de marcas de tiempo LRU (quien llama también debe llamar a advance_tick)
    // En realidad, quien llama llamará a advance_tick antes; asumimos que tick_counter es el tick actual
    //Comprobar residente
        if (is_resident_and_touch(pid,page)) {
            return {true, -1};
        } else {
            int fid = load_page(pid,page);
            return {false, fid};
        }
    }

    // estadísticas getters
    int get_total_page_faults() const { return total_page_faults; }
    int get_total_replacements() const { return total_replacements; }

    // Mostrar estado de los frames
    void dump_frames() const {
        cout << "Frames (id : pid,page,loaded_at,last_access):\n";
        for (auto &f: frames) {
            cout << f.fid << " : ";
            if (f.pid == -1) cout << "<free>\n";
            else cout << f.pid << "," << f.page << " (l@" << f.loaded_at_tick << " a@" << f.last_access_tick << ")\n";
        }
    }
};


// Planificador (dos algoritmos): RR y SJF no expropiativo

enum class CPUPolicy { RR, SJF_NONPREEMPTIVE };

class Scheduler {
private:
    CPUPolicy policy;
    int quantum;
    int current_tick = 0;
    int next_pid = 1;

    unordered_map<int, PCB> procs;
    deque<int> ready_q;          // cola de listos (para RR)
    // Para SJF, analizaremos los procesos para seleccionar el más corto cuando haya CPU libre.

    optional<int> running_pid;
    int rr_slice_used = 0; // Unidades utilizadas en la porción RR actual

public:
    Scheduler(CPUPolicy p = CPUPolicy::RR, int q=2): policy(p), quantum(q) {}

    // crea el proceso
    int create_process(int burst, int npages = 4, const vector<int> &trace = {}) {
        int pid = next_pid++;
        PCB pcb(pid, burst, current_tick, npages);
        if (!trace.empty()) pcb.trace = trace;
        pcb.estado = Estado::READY;
        procs[pid] = pcb;
        ready_q.push_back(pid);
        cout << "[tick " << current_tick << "] CREATED pid=" << pid << " burst=" << burst << " pages=" << npages << "\n";
        return pid;
    }

    // "mata" el proceso
    void kill_process(int pid) {
        if (procs.find(pid) == procs.end()) { cout << "pid not found\n"; return; }
        auto &p = procs[pid];
        p.estado = Estado::TERMINATED;
        p.fin_tick = current_tick;
    // Eliminar de ready_q si está presente
        ready_q.erase(remove(ready_q.begin(), ready_q.end(), pid), ready_q.end());
        if (running_pid && running_pid.value() == pid) {
            running_pid.reset();
            rr_slice_used = 0;
        }
        cout << "[tick " << current_tick << "] KILLED pid=" << pid << "\n";
    }

    // cambia politica de la CPU
    void set_policy(CPUPolicy p, int q = 2) {
        policy = p;
        quantum = q;
        // restablecer el estado de tiempo de ejecución
        running_pid.reset();
        rr_slice_used = 0;
        cout << "Scheduler set to " << (policy==CPUPolicy::RR ? "RR" : "SJF_nonpreemptive") << " quantum=" << quantum << "\n";
    }

    CPUPolicy get_policy() const { return policy; }

    // Función auxiliar para seleccionar el siguiente proceso cuando haya CPU libre
    optional<int> schedule_next() {
        if (policy == CPUPolicy::RR) {
            if (!running_pid && !ready_q.empty()) {
                int pid = ready_q.front(); ready_q.pop_front();
                return pid;
            }
            return {};
        } else { // SJF no expropiativo
            // elige el proceso READY con el rafaga_restante más pequeño
            int best = -1;
            int best_burst = INT_MAX;
            for (auto &kv : procs) {
                auto &p = kv.second;
                if (p.estado == Estado::READY) {
                    if (p.rafaga_restante < best_burst) {
                        best_burst = p.rafaga_restante;
                        best = p.pid;
                    }
                }
            }
            if (best != -1) {
                // Eliminar "best" del contenedor de la lista de READY (ready_q aún podría contenerlo)
                ready_q.erase(remove(ready_q.begin(), ready_q.end(), best), ready_q.end());
                return best;
            }
            return {};
        }
    }

    // Avanza un tick: ejecuta 1 unidad si hay proceso corriendo
    optional<int> tick() {
        // si no hay proceso corriendo, planifica uno
        if (!running_pid) {
            auto next = schedule_next();
            if (next) {
                running_pid = next.value();
                auto &p = procs[running_pid.value()];
                p.estado = Estado::RUNNING;
                if (p.inicio_tick == -1) p.inicio_tick = current_tick;
                rr_slice_used = 0;
                cout << "[tick " << current_tick << "] SCHEDULE pid=" << running_pid.value() << "\n";
            }
        }

        // incrementar tiempo de espera para procesos en READY
        for (auto &kv : procs) {
            auto &p = kv.second;
            if (p.estado == Estado::READY) p.espera_acumulada++;
        }

        optional<int> ran_pid = {};
        if (running_pid) {
            int pid = running_pid.value();
            ran_pid = pid;
            auto &p = procs[pid];
            // ejecutar 1 unidad
            p.rafaga_restante--;
            cout << "[tick " << current_tick << "] RUN pid=" << pid << " rem=" << p.rafaga_restante << "\n";
            // verifica terminación
            if (p.rafaga_restante <= 0) {
                p.estado = Estado::TERMINATED;
                p.fin_tick = current_tick + 1; // finaliza al final de este ciclo
                cout << "[tick " << current_tick << "] EXIT pid=" << pid << "\n";
                running_pid.reset();
                rr_slice_used = 0;
            } else {
                // si RR, verifica el quantum
                if (policy == CPUPolicy::RR) {
                    rr_slice_used++;
                    if (rr_slice_used >= quantum) {
                        // expropiación
                        p.estado = Estado::READY;
                        ready_q.push_back(pid);
                        cout << "[tick " << current_tick << "] PREEMPT pid=" << pid << "\n";
                        running_pid.reset();
                        rr_slice_used = 0;
                    }
                } else {
                    }
            }
        }

        current_tick++;
        return ran_pid;
    }

    // ejecutar n ticks (ciclos)
    void run_ticks(int n, function<void(int)> on_run_pid = nullptr) {
        for (int i = 0; i < n; ++i) {
            auto ran = tick();
            if (ran && on_run_pid) on_run_pid(ran.value());
        }
    }


    const unordered_map<int, PCB>& get_processes() const { return procs; }


    unordered_map<int, PCB>& get_processes_mut() { return procs; }

    // hace los procesos READY (usados en creation)
    void make_ready(int pid) {
        if (procs.find(pid) == procs.end()) return;
        auto &p = procs[pid];
        if (p.estado == Estado::NEW) p.estado = Estado::READY;
        // evita duplicados en ready_q
        if (find(ready_q.begin(), ready_q.end(), pid) == ready_q.end())
            ready_q.push_back(pid);
    }

    // Mostrar tabla de procesos
    void ps() const {
        cout << "PID\tESTADO\tRAFAGA\tNPAGES\tARR\tINI\tFIN\tESPERA\tPF\n";
        for (auto &kv : procs) {
            auto &p = kv.second;
            cout << p.pid << "\t" << estado_to_str(p.estado) << "\t"
                 << p.rafaga_restante << "\t" << p.npages << "\t"
                 << p.llegada_tick << "\t" << p.inicio_tick << "\t"
                 << p.fin_tick << "\t" << p.espera_acumulada << "\t"
                 << p.page_faults << "\n";
        }
    }

    int get_tick() const { return current_tick; }
};


// CLI + Integración

static string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

static vector<int> parse_trace(const string &s) {
    vector<int> out;
    string cur;
    for (char c : s) {
        if (c==',' || isspace((unsigned char)c)) {
            if (!cur.empty()) { out.push_back(stoi(cur)); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(stoi(cur));
    return out;
}

int main() {
    cout << "=== OS Simulator (SJF non-preemptive + LRU) ===\n";
    cout << "Nota: scheduler default = RR quantum=2, page policy default = FIFO\n";


    Scheduler sched(CPUPolicy::RR, 2);
    MemoryManager mem(8, ReplPolicy::FIFO); 

    string line;
    while (true) {
        cout << ">> ";
        if (!getline(cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd; ss >> cmd;

        if (cmd == "help") {
            cout << "Comandos:\n"
                 << "  new <burst> [npages] [trace_comma_sep]   -> crear proceso\n"
                 << "     e.g. new 10 4 0,1,2,1  (burst=10,npages=4,trace)\n"
                 << "  ps                                       -> listar procesos\n"
                 << "  tick                                     -> avanzar 1 tick\n"
                 << "  run N                                    -> ejecutar N ticks\n"
                 << "  kill PID                                 -> matar proceso\n"
                 << "  set_sched RR <quantum>                   -> Round-Robin\n"
                 << "  set_sched SJF                            -> SJF no-expropiativo\n                 "
                 << "  set_pagemode FIFO|LRU <nframes>          -> set replacement and optionally resize frames\n"
                 << "  memstat                                  -> mostrar frames y stats\n"
                 << "  help                                     -> mostrar ayuda\n"
                 << "  exit                                     -> salir\n";
        }
        else if (cmd == "exit") {
            cout << "Saliendo...\n";
            break;
        }
        else if (cmd == "new") {
            int burst; if (!(ss >> burst)) { cout << "new requires burst\n"; continue; }
            int np = 4;
            if (ss >> np) {


                // Se consumió np; el siguiente token podría ser una cadena de seguimiento (resto de la línea)
                string rest; getline(ss, rest);
                rest = trim(rest);
                if (!rest.empty()) {
                // Analizar enteros separados por comas
                    auto tr = parse_trace(rest);
                    int pid = sched.create_process(burst, np, tr);
                    sched.make_ready(pid);
                } else {
                    int pid = sched.create_process(burst, np, {});
                    sched.make_ready(pid);
                }
            } else {


            // solo se dio una ráfaga
                int pid = sched.create_process(burst, 4, {});
                sched.make_ready(pid);
            }
        }
        else if (cmd == "ps") {
            sched.ps();
        }
        else if (cmd == "kill") {
            int pid; if (!(ss >> pid)) { cout << "kill requires pid\n"; continue; }
            sched.kill_process(pid);
        }
        else if (cmd == "set_sched") {
            string arg; ss >> arg;
            if (arg == "RR") {
                int q=2; ss >> q;
                sched.set_policy(CPUPolicy::RR, q);
            } else if (arg == "SJF") {
                sched.set_policy(CPUPolicy::SJF_NONPREEMPTIVE, 0);
            } else {
                cout << "Unknown scheduler. Use RR or SJF\n";
            }
        }
        else if (cmd == "set_pagemode") {
            string arg; ss >> arg;
            if (arg == "FIFO") {
                int newframes = -1; if (ss >> newframes) {
                    mem = MemoryManager(newframes, ReplPolicy::FIFO);
                } else mem.set_policy(ReplPolicy::FIFO);
                cout << "Page replacement = FIFO\n";
            } else if (arg == "LRU") {
                int newframes = -1; if (ss >> newframes) {
                    mem = MemoryManager(newframes, ReplPolicy::LRU);
                } else mem.set_policy(ReplPolicy::LRU);
                cout << "Page replacement = LRU\n";
            } else {
                cout << "Usage: set_pagemode FIFO|LRU [nframes]\n";
            }
        }
        else if (cmd == "memstat") {
            cout << "Memory stats at tick " << sched.get_tick() << "\n";
            cout << "Total page faults: " << mem.get_total_page_faults()
                 << " total replacements: " << mem.get_total_replacements() << "\n";
            mem.dump_frames();
        }
        else if (cmd == "tick") {
            // avanzar la memoria del tick primero
            mem.advance_tick();
            // El tick del planificador devuelve el pid que ejecutó este tick
            auto ran = sched.tick();
            if (ran) {
                int pid = ran.value();
        // Realizar acceso a memoria para el pid: elegir página de la traza o aleatoriamente.
                auto &procs = sched.get_processes_mut();
                if (procs.find(pid) != procs.end()) {
                    PCB &p = procs[pid];
                    int page = 0;
                    if (!p.trace.empty()) {
                        if (p.trace_pos >= (int)p.trace.size()) p.trace_pos = 0;
                        page = p.trace[p.trace_pos++];
                        if (page < 0 || page >= p.npages) page = page % p.npages;
                    } else {
                        // pagina random
                        std::uniform_int_distribution<int> dist(0, max(0,p.npages-1));
                        page = dist(rng);
                    }
                    auto res = mem.access_page(pid, page);
                    if (!res.first) {
                        // fallo de pagina
                        p.page_faults++;
                        cout << "[tick " << sched.get_tick()-1 << "] PAGE_FAULT pid=" << pid << " page=" << page << " loaded in frame=" << res.second << "\n";
                    } else {
                        cout << "[tick " << sched.get_tick()-1 << "] HIT pid=" << pid << " page=" << page << "\n";
                    }
                }
            }
        }
        else if (cmd == "run") {
            int n; if (!(ss >> n)) { cout << "run requires a number\n"; continue; }
            for (int i = 0; i < n; ++i) {
                mem.advance_tick();
                auto ran = sched.tick();
                if (ran) {
                    int pid = ran.value();
                    auto &procs = sched.get_processes_mut();
                    if (procs.find(pid) != procs.end()) {
                        PCB &p = procs[pid];
                        int page = 0;
                        if (!p.trace.empty()) {
                            if (p.trace_pos >= (int)p.trace.size()) p.trace_pos = 0;
                            page = p.trace[p.trace_pos++];
                            if (page < 0 || page >= p.npages) page = page % p.npages;
                        } else {
                            std::uniform_int_distribution<int> dist(0, max(0,p.npages-1));
                            page = dist(rng);
                        }
                        auto res = mem.access_page(pid, page);
                        if (!res.first) {
                            p.page_faults++;
                            cout << "[tick " << sched.get_tick()-1 << "] PAGE_FAULT pid=" << pid << " page=" << page << " loaded in frame=" << res.second << "\n";
                        } else {
                            cout << "[tick " << sched.get_tick()-1 << "] HIT pid=" << pid << " page=" << page << "\n";
                        }
                    }
                }
            }
        }
        else {
            cout << "Comando desconocido. Escribe help.\n";
        }
    }

    return 0;
}


