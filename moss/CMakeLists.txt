idf_build_get_property(target IDF_TARGET)


# Add a file glob for this.
idf_component_register(SRCS "src/moss.c" "src/startup.c" "src/interrupt.c" "src/scheduler.c" "src/asm/xtensa_vectors.S" "src/asm/moss_portasm.S" "src/synch.c"
			    REQUIRES esp_timer esp_hw_support heap esp_app_format vfs hal soc xtensa
			    INCLUDE_DIRS "include")	

set(SECTIONSLD ${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in)
set(SECTIONSLD_BACKUP ${esp-idf_SOURCE_DIR}/components/esp_system/ld/esp32/sections.ld.in.mossbackup)

set(XTENSAVEC ${esp-idf_SOURCE_DIR}/components/freertos/FreeRTOS-Kernel/portable/xtensa/xtensa_vectors.S)
set(XTENSAVEC_BACKUP ${esp-idf_SOURCE_DIR}/components/freertos/FreeRTOS-Kernel/portable/xtensa/xtensa_vectors.S.mossbackup)


# Do some really hacky stuff right over here to override the vector table symbols
if(NOT EXISTS ${SECTIONSLD_BACKUP})
	file(RENAME 
		${SECTIONSLD}
		${SECTIONSLD_BACKUP})
endif()


if(NOT EXISTS ${XTENSAVEC_BACKUP})
	file(RENAME 
		${XTENSAVEC}
		${XTENSAVEC_BACKUP})

	file(TOUCH 
		${XTENSAVEC})
endif()


file(COPY_FILE 
	linkerscript_override/sections.ld.in
	${SECTIONSLD})

add_custom_target("moss_clean")
add_custom_command(TARGET "moss_clean"
					POST_BUILD
					COMMAND mv ${XTENSAVEC_BACKUP} ${XTENSAVEC} && mv ${SECTIONSLD_BACKUP} ${SECTIONSLD}

)

target_compile_options(${COMPONENT_LIB} PRIVATE "-fsanitize=undefined" "-fno-sanitize=shift-base")
