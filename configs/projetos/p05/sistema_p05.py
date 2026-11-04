import sys
import os
import argparse
import m5
from m5.objects import *

# Adiciona o diretório src ao path para encontrar o ImbalanceDetector
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__),
                                                '../../../src')))
from projetos.p05.ImbalanceDetector import ImbalanceDetector

ap = argparse.ArgumentParser()
ap.add_argument("--cores", type=int, default=4)
ap.add_argument("--cpu", choices=["timing", "o3"], default="timing")
ap.add_argument("--period", default="10us")
ap.add_argument("--series", default="serie.csv")
ap.add_argument("--no-detector", action="store_true",
                help="executa sem ImbalanceDetector (linha de base)")
ap.add_argument("--max-ticks", type=int, default=0)
ap.add_argument("cmd", nargs="+")
args = ap.parse_args()

system = System()

# Relógio e domínio de tensão
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Modo de memória e endereçamento
system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MB")]

# Barramento de sistema (memória)
system.membus = SystemXBar()
system.system_port = system.membus.cpu_side_ports

# Barramento L2 (entre L1s e L2)
system.l2bus = L2XBar()

# CPUs com L1 privadas conectadas ao L2 bus
if args.cpu == "o3":
    system.cpu = [X86O3CPU(cpu_id=i) for i in range(args.cores)]
else:
    system.cpu = [X86TimingSimpleCPU(cpu_id=i) for i in range(args.cores)]

for i in range(args.cores):
    cpu = system.cpu[i]

    # L1 privadas (instruction e data)
    cpu.icache = L1ICache(size="32kB", assoc=8)
    cpu.dcache = L1DCache(size="32kB", assoc=8)
    cpu.icache.connectCPU(cpu)
    cpu.dcache.connectCPU(cpu)
    cpu.icache.connectBus(system.l2bus)
    cpu.dcache.connectBus(system.l2bus)

    # Controladores de interrupção (x86 precisa)
    cpu.createInterruptController()
    cpu.interrupts[0].pio = system.membus.mem_side_ports
    cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
    cpu.interrupts[0].int_responder = system.membus.mem_side_ports

# L2 compartilhada: conecta no l2bus (lado CPU) e no membus (lado memória)
system.l2cache = L2Cache(size="256kB", assoc=8)
system.l2cache.connectCPUSideBus(system.l2bus)
system.l2cache.connectMemSideBus(system.membus)

# Controlador de memória
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# Detector (opcional)
if not args.no_detector:
    system.detector = ImbalanceDetector(
        cpus=system.cpu,
        period=args.period,
        series_file=args.series)

# Processo de workload
process = Process()
process.cmd = args.cmd
for cpu in system.cpu:
    cpu.workload = process
    cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()
evento = (m5.simulate(args.max_ticks) if args.max_ticks else m5.simulate())
print("Fim @ tick %d: %s" % (m5.curTick(), evento.getCause()))