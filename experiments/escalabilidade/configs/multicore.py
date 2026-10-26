import argparse
import m5
from m5.objects import *

parser = argparse.ArgumentParser()
parser.add_argument("--num-cpus", type=int, required=True)
parser.add_argument("--cmd", required=True)
parser.add_argument("--threads", type=int, required=True)
args = parser.parse_args()

if args.threads > args.num_cpus:
    parser.error("Use no máximo uma thread de trabalho por CPU simulado")

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "2GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = "timing"
system.mem_ranges = [AddrRange("1GiB")]

system.cpu = [TimingSimpleCPU(cpu_id=i) for i in range(args.num_cpus)]
system.membus = SystemXBar()

# L1 privadas. A memória principal é compartilhada.
for cpu in system.cpu:
    cpu.icache = Cache(size="32KiB", assoc=4, tag_latency=1,
                       data_latency=1, response_latency=1,
                       mshrs=4, tgts_per_mshr=20)
    cpu.dcache = Cache(size="32KiB", assoc=4, tag_latency=1,
                       data_latency=1, response_latency=1,
                       mshrs=4, tgts_per_mshr=20)
    cpu.icache.cpu_side = cpu.icache_port
    cpu.dcache.cpu_side = cpu.dcache_port
    cpu.icache.mem_side = system.membus.cpu_side_ports
    cpu.dcache.mem_side = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

process = Process()
process.executable = args.cmd	
process.cmd = [args.cmd]
process.env = ["OMP_NUM_THREADS=" + str(args.threads),
               "OMP_DYNAMIC=FALSE",
               "OMP_WAIT_POLICY=active",
               "GOMP_SPINCOUNT=infinite"]

system.workload = SEWorkload.init_compatible(process.executable)

# O mesmo processo multithread é inicializado na CPU 0.
# As threads podem ser distribuídas pelos CPUs pelo suporte de threading do SE.
# system.cpu[0].workload = process
# system.cpu[0].createThreads()

for cpu in system.cpu:
    cpu.workload = process
    cpu.createThreads()
    cpu.createInterruptController()

    cpu.interrupts[0].pio = system.membus.mem_side_ports
    cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
    cpu.interrupts[0].int_responder = system.membus.mem_side_ports

root = Root(full_system=False, system=system)
m5.instantiate()
event = m5.simulate()
print("Fim:", event.getCause(), "tick:", m5.curTick())
