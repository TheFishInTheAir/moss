TARGET_EXEC ?= cosexec
OS_ARCHIVE ?= libcos.a

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src


ESP_IDF_DIR := ${HOME}/Documents/libraries/esp/esp-idf
ESP_IDF_TOOL_DIR := ${HOME}/.espressif/tools/xtensa-esp32-elf/esp-2022r1-11.2.0/xtensa-esp32-elf/bin

IDFDEP_SUBPROJECT ?= ./idf_dependencies
IDFDEP_BUILD_COMPONENTS ?= ${BUILD_DIR}/idfdep/build/esp-idf
IDF_COMPONENTS ?= ${ESP_IDF_DIR}/components

SRCS := $(shell find $(SRC_DIRS) -name "*.c")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
#LIB_DIR := -L/usr/local/Cellar/llvm/14.0.6_1/lib/
#LIBS := -lLLVM-C

LDFLAGS := $(LIB_DIR) $(LIBS)


CC  	:= ${ESP_IDF_TOOL_DIR}/xtensa-esp32-elf-gcc
AS  	:= ${ESP_IDF_TOOL_DIR}/xtensa-esp32-elf-as
LD		:= ${ESP_IDF_TOOL_DIR}/xtensa-esp32-elf-ld
AR		:= ${ESP_IDF_TOOL_DIR}/xtensa-esp32-elf-ar
OBJDUMP := ${ESP_IDF_TOOL_DIR}/xtensa-esp32-elf-objdump
LDGEN 	:= ${ESP_IDF_DIR}/tools/ldgen/ldgen.py

INC_DIRS := ./include ${IDF_COMPONENTS}/soc/esp32/include \
 ${IDF_COMPONENTS}/esp_common/include \
 ${IDF_COMPONENTS}/bootloader_support/include \
 ${IDF_COMPONENTS}/bootloader_support/bootloader_flash/include \
 ${IDF_COMPONENTS}/xtensa/include \
 ${IDF_COMPONENTS}/xtensa/esp32/include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LIBRARY_LFRAGS := ${IDF_COMPONENTS}/soc/linker.lf \
 ${IDF_COMPONENTS}/esp_system/linker.lf \
 ${IDF_COMPONENTS}/esp_common/common.lf \
 ${IDF_COMPONENTS}/esp_rom/linker.lf \
 ${IDF_COMPONENTS}/hal/linker.lf \
 ${IDF_COMPONENTS}/log/linker.lf \
 ${IDF_COMPONENTS}/newlib/newlib.lf \
 ${IDF_COMPONENTS}/newlib/system_libs.lf \
 ${IDF_COMPONENTS}/spi_flash/linker.lf \
 ${IDF_COMPONENTS}/esp_common/soc.lf \
 ${IDF_COMPONENTS}/xtensa/linker.lf

LIBRARY_ARCHIVES := ${IDFDEP_BUILD_COMPONENTS}/soc/libsoc.a \
${IDFDEP_BUILD_COMPONENTS}/esp_common/libesp_common.a \
${IDFDEP_BUILD_COMPONENTS}/esp_system/libesp_system.a \
${IDFDEP_BUILD_COMPONENTS}/esp_rom/libesp_rom.a \
${IDFDEP_BUILD_COMPONENTS}/spi_flash/libspi_flash.a \
${IDFDEP_BUILD_COMPONENTS}/log/liblog.a \
${IDFDEP_BUILD_COMPONENTS}/esp_timer/libesp_timer.a \
${IDFDEP_BUILD_COMPONENTS}/efuse/libefuse.a \
${IDFDEP_BUILD_COMPONENTS}/hal/libhal.a \
${IDFDEP_BUILD_COMPONENTS}/newlib/libnewlib.a \
${IDFDEP_BUILD_COMPONENTS}/bootloader_support/libbootloader_support.a \
${IDFDEP_BUILD_COMPONENTS}/xtensa/libxtensa.a

LFRAGS := $(shell find . -name *.lf) ${LIBRARY_LFRAGS}
LDTEMPLATE := ${ESP_IDF_DIR}/components/esp_system/ld/esp32/sections.ld.in
LDGEN_LIBRARIES := ${BUILD_DIR}/ldgen_libraries
LDOUTPUT := ${BUILD_DIR}/link.ld

LINKERSCRIPTS := ./linkerscripts/memory.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.api.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.libgcc.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.newlib-data.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.newlib-funcs.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.newlib-locale.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.newlib-nano.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.newlib-time.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.spiflash.ld \
${IDF_COMPONENTS}/esp_rom/esp32/ld/esp32.rom.syscalls.ld \
${LDOUTPUT} 

LINKERSCRIPTS_FLAGS := $(addprefix -T,$(LINKERSCRIPTS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

all: $(BUILD_DIR)/$(TARGET_EXEC)


debug: CPPFLAGS += -DDEBUG -g
debug: CFLAGS += -DDEBUG -g
debug: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(BUILD_DIR)/$(OS_ARCHIVE) ldgen idf_dependencies
	$(LD) $(BUILD_DIR)/$(OS_ARCHIVE) ${LIBRARY_ARCHIVES} ${LINKERSCRIPTS_FLAGS} -o $@ $(LDFLAGS)

$(BUILD_DIR)/$(OS_ARCHIVE): $(OBJS)
	$(AR) r $@ $(OBJS)

${LDGEN_LIBRARIES}: ${BUILD_DIR}/${OS_ARCHIVE}
	echo "${BUILD_DIR}/${OS_ARCHIVE} ${LIBRARY_ARCHIVES}" | tr " " "\n" > ${LDGEN_LIBRARIES}

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@ 

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

ldgen: $(LFRAGS) ${LDGEN_LIBRARIES} idf_dependencies
	$(LDGEN) -i $(LDTEMPLATE) -f $(LFRAGS) -k ./linkerscripts/KConfig -c ${ESP_IDF_DIR}/tools/ldgen/samples/sdkconfig --libraries-file ${LDGEN_LIBRARIES} --objdump ${OBJDUMP} -o ${LDOUTPUT}

idf_dependencies: $(shell find ${IDFDEP_SUBPROJECT} -type f)
	$(MKDIR_P) ${BUILD_DIR}/idfdep/build
	pushd ${BUILD_DIR}/idfdep && \
	cp ../../${IDFDEP_SUBPROJECT}/CMakeLists.txt CMakeLists.txt 
	-idf.py -C ${BUILD_DIR}/idfdep -B ${BUILD_DIR}/idfdep/build all  # This command will always fail as we arent building a project
	@echo ^^^ OK SO JUST IGNORE ALL THE ABOVE ERRORS, LIKE ITS COOL, DONT WORRY ^^^



# c++ source
#$(BUILD_DIR)/%.cpp.o: %.cpp
#	$(MKDIR_P) $(dir $@)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
