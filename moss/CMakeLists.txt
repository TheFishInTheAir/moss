# Add a file glob for this.
idf_component_register(SRCS "src/moss.c" "src/startup.c" "src/interrupt.c" "src/scheduler.c" "src/asm/xtensa_vectors.S" "src/asm/moss_portasm.S" "src/synch.c"
			    REQUIRES esp_timer esp_hw_support heap esp_app_format vfs hal soc xtensa
			    INCLUDE_DIRS "include")	


# Do some really hacky stuff right over here to override the vector table symbols
if(NOT EXISTS ${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in.mossbackup)
	file(RENAME 
		${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in 
		${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in.mossbackup)
endif()

file(COPY_FILE 
	linkerscript_override/sections.ld.in
	${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in )