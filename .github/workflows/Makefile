MyAVR.source.compiler = avr-c++
MyAVR.source.libraries = 
MyAVR.device.type = attiny25
MyAVR.device.frequency = 1000000UL

ifeq "$(MyAVR.source.compiler)" "$(notdir $(MyAVR.source.compiler))"
A.compiler.dir =
else
A.compiler.dir = $(dir $(MyAVR.source.compiler))
endif
A.libraries = $(addprefix -I,$(wildcard $(MyAVR.source.libraries:=/.) $(MyAVR.source.libraries:=/*/.) $(MyAVR.source.libraries:=/*/*/.)))
A.output.dir = .build
A.elf = $(A.output.dir)/output.elf
A.list = $(A.output.dir)/output.lst
A.src = $(call F.dep,$(sort $(wildcard *.c) $(wildcard *.cpp)))
A.obj = $(addprefix $(A.output.dir)/obj/,$(addsuffix .o,$(basename $(A.src))))

F.dep.base = $(basename $1 $(filter %.h %.hpp,$(shell $(E.get.dep) $1)))
F.dep.1lvl = $(sort $(wildcard $(addsuffix .cpp,$(call F.dep.base,$1))) $(wildcard $(addsuffix .c,$(call F.dep.base,$1))))
F.dep = $(if $(filter-out '$1','$(call F.dep.1lvl,$1)'),$(call F.dep,$(call F.dep.1lvl,$1)),$1)

E.compiler = "$(MyAVR.source.compiler)" -DF_CPU=$(MyAVR.device.frequency) -mmcu=$(MyAVR.device.type) -pipe $(A.libraries)
E.get.dep = $(E.compiler) -MM
E.compile = $(E.compiler) -std=c++14 -g -Os -Wall -Wextra -pedantic -c -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -flto
E.link = $(E.compiler) -Wall -Wextra -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections -lm
E.make.list = "$(A.compiler.dir)avr-objdump" --disassemble --source --line-numbers --demangle
E.get.size = "$(A.compiler.dir)avr-size" -A

.PHONY : build

build : $(A.list)
	@$(E.get.size) $(A.elf)

$(A.list) : $(A.elf)
	@mkdir -p $(@D)
	$(info ===== Making $@)
	@$(E.make.list) $^ > $@
	$(info )

$(A.elf) : $(A.obj)
	@mkdir -p $(@D)
	$(info ===== Making $@)
	@$(E.link) $^ -o $@
	$(info )

-include $(A.obj:.o=.d)

$(A.output.dir)/obj/%.o : %.c
	@mkdir -p $(@D)
	$(info ===== Making $@)
	@$(E.compile) $< -o $@
	$(info )

$(A.output.dir)/obj/%.o : %.cpp
	@mkdir -p $(@D)
	$(info ===== Making $@)
	@$(E.compile) $< -o $@
	$(info )

