file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "flash_project_args"
  "project_elf_src_esp32.c"
  "test_app.bin"
  "test_app.map"
  "CMakeFiles/test_app.elf.dir/project_elf_src_esp32.c.obj"
  "CMakeFiles/test_app.elf.dir/project_elf_src_esp32.c.obj.d"
  "project_elf_src_esp32.c"
  "test_app.elf"
  "test_app.elf.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/test_app.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
