import m5
from m5.objects import *

system = System()
system.clk_domain = SrcClockDomain(clock="1GHz", voltage_domain=VoltageDomain())
system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MiB")]
system.membus = SystemXBar()
system.system_port = system.membus.cpu_side_ports

system.cpu = [X86TimingSimpleCPU(cpu_id=i) for i in range(2)]

for cpu in system.cpu:
    cpu.icache_port = system.membus.cpu_side_ports
    cpu.dcache_port = system.membus.cpu_side_ports
    cpu.createInterruptController()
    cpu.interrupts[0].pio = system.membus.mem_side_ports
    cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
    cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.mem_ctrl = MemCtrl(dram=DDR3_1600_8x8(range=system.mem_ranges[0]))
system.mem_ctrl.port = system.membus.mem_side_ports

binary = "tests/test-progs/hello/bin/x86/linux/hello"
system.workload = SEWorkload.init_compatible(binary)
process = Process(cmd=[binary])
for cpu in system.cpu:
    cpu.workload = process
    cpu.createThreads()

system.detector = ImbalanceDetector(cpus=system.cpu, period="10us")

root = Root(full_system=False, system=system)
m5.instantiate()

m5.simulate(1000)
