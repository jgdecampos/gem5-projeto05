import argparse
import m5
from m5.objects import *

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
# TODO(aluno): relógio, memória, barramento, como no capítulo 3.
# TODO(aluno): args.cores CPUs com L1I e L1D privadas; L2
#              compartilhada; controlador de memória.

if not args.no_detector:
    system.detector = ImbalanceDetector(cpus=system.cpu,
                                        period=args.period,
                                        series_file=args.series)

# TODO(aluno): processo com args.cmd, atribuído a todas as CPUs.

root = Root(full_system=False, system=system)
m5.instantiate()
evento = (m5.simulate(args.max_ticks) if args.max_ticks
          else m5.simulate())
print(f"Fim @ tick {m5.curTick()}: {evento.getCause()}")
