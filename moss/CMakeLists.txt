# Add a file glob for this.
idf_component_register(SRCS "src/moss.c" "src/startup.c" "src/interrupt.c" "src/scheduler.c"
			    REQUIRES esp_timer esp_hw_support heap esp_app_format vfs hal soc
			    INCLUDE_DIRS "include")	
