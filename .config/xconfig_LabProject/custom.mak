## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,28FP linker.cmd package/cfg/LabProject_p28FP.o28FP

# To simplify configuro usage in makefiles:
#     o create a generic linker command file name 
#     o set modification times of compiler.opt* files to be greater than
#       or equal to the generated config header
#
linker.cmd: package/cfg/LabProject_p28FP.xdl
	$(SED) 's"^\"\(package/cfg/LabProject_p28FPcfg.cmd\)\"$""\"C:/Users/kyrun/OneDrive/Desktop/Embedded/LabProject/.config/xconfig_LabProject/\1\""' package/cfg/LabProject_p28FP.xdl > $@
	-$(SETDATE) -r:max package/cfg/LabProject_p28FP.h compiler.opt compiler.opt.defs
