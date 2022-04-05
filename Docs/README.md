# What is VT and ANSI Emulation?

The terms "VT emulation" and "ANSI terminal" will pop up when talking about serial communication.

**VT emulation** comes from the VT terminals by DEC (Digital Equipment Corporation). The first popular DEC terminal was the VT-52. It was followed by the VT-100, one of the first terminals to support standard sequences from ANSI. The file
VT100.pdf has some technical information about the codes the VT-100 terminal accepted. 
Obs: This is a scan of a second (or third) generation photocopy.

The **ANSI Standard** define the functions of a few control code (BEL, BS, HT, LF, FF and CR) and
many "CSI sequences". "CSI" means "Control Sequence Introducer", ESC [ or 0x9B. A CSI sequence
usually has the form "CSI parameters letter", where parameter are zero or more decimal numbers
separated by ';' (VT100.pdf and wikipedia have a more complete and formal definition).

Besides the VT-100, another important use of ANSI sequences was in ANSY.SYS, a device driver
included in MS-DOS version 2 and latter. The file NANSI.txt is the documentation for NANSI.SYS,
an (now) open source replacement for ANSI.SYS.

