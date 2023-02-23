
About
------
(c) Matias Henttunen 2023

EAP network variables read/write utility.
Probably incomplete, but works for simple read/write network variables
on Beckhoff systems

Beckhoff EAP read/write tool
----------------------------
EAP = EtherCat Automation Protocol
This tool can be used for reading/writing EAP variables from a linux system.

See more on https://www.ethercat.org/en/downloads/downloads_BB6D7FF18F2B47DDB3474168D50EE864.htm

Build instructions
------------------
make -C dependencies/moderncpp.git lib
cd src; make


dependencies
-------------
nlohman::json
libzmq

Notes
--------
This software will most probably not work on non linux systems without modifications

EAP traffic is flooding data, so ensure your network can handle this.

Twincat3 has extended the format of networkvariables
[ publish id|cnt | cycle | res| eapSM ]

[ res | toggle | err | stat ]
