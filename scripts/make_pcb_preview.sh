#!/usr/bin/env bash
cd pcb
pcbdraw plot --side front -l custom --components -m remap.json -s set-black-hasl Gaggimate.kicad_pcb ../docs/assets/pcb_top.png
pcbdraw plot --side back -l custom --components -m remap.json -s set-black-hasl Gaggimate.kicad_pcb ../docs/assets/pcb_bottom.png
sips -r 180 ../docs/assets/pcb_bottom.png
