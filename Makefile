include Makefile.head

all: Image

Image:
	$(Q)(cd $(LINUX_SRC); make $@)

clean:
	$(Q)(cd $(ROOTFS_DIR); make $@)
	$(Q)(cd $(CALLGRAPH_DIR); make $@)
	$(Q)(cd $(LINUX_SRC); make $@)
	$(Q)rm -rf bochsout.txt

distclean: clean
	$(Q)(cd $(LINUX_SRC); make $@)

# Test on emulators with different prebuilt rootfs
include Makefile.emu

# Tags for source code reading
include Makefile.tags

# For Call graph generation
include Makefile.cg

help:
	@echo "-----Linux 0.11 Lab (http://tinylab.org/linux-0.11-lab)-----"
	@echo ""
	@echo "     :: Compile ::"
	@echo ""
	@echo "     make --generate a kernel floppy Image with a fs on hda1"
	@echo "     make clean -- clean the object files"
	@echo "     make distclean -- only keep the source code files"
	@echo ""
	@echo "     :: Test ::"
	@echo ""
	@echo "     make start -- start the kernel in vm (qemu/bochs)"
	@echo "     make start-fd -- start the kernel with fs in floppy"
	@echo "     make start-hd -- start the kernel with fs in hard disk"
	@echo "     make start-hd G=0 -- start with curses based terminal, instead of SDL"
	@echo ""
	@echo ""
	@echo "     :: Debug ::"
	@echo ""
	@echo "     make debug -- debug the kernel in qemu/bochs & gdb at port 1234"
	@echo "     make debug-fd -- debug the kernel with fs in floppy"
	@echo "     make debug-hd -- debug the kernel with fs in hard disk"
	@echo ""
	@echo "     make debug DST=boot/bootsect.sym -- debug bootsect"
	@echo "     make debug DST=boot/setup.sym    -- debug setup"
	@echo ""
	@echo "     make switch             -- switch the emulator: qemu and bochs"
	@echo "     make boot VM=qemu|bochs -- switch the emulator: qemu and bochs"
	@echo ""
	@echo "     :: Read ::"
	@echo ""
	@echo "     make cscope -- genereate the cscope index databases"
	@echo "     make tags -- generate the tag file"
	@echo "     make cg -- generate callgraph of the default main entry"
	@echo "     make cg f=func d=dir|file b=browser -- generate callgraph of func in file/directory"
	@echo ""
	@echo "     :: More ::"
	@echo ""
	@echo "     >>> README.md <<<"
	@echo ""
	@echo "     ~ Enjoy It ~"
	@echo ""
	@echo "-----Linux 0.11 Lab (http://tinylab.org/linux-0.11-lab)-----"
	@echo ""
	@echo "--->  Linux Kernel Lab (http://tinylab.org/linux-lab)   <---"
