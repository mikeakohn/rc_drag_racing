idf_component_register(
  SRCS
    drag_race_esp32.cxx
    DragRace.cxx
    DragRaceEnd.cxx
    DragRaceStart.cxx
    Network.cxx
    NetworkEnd.cxx
    NetworkStart.cxx
  INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)

spiffs_create_partition_image(storage ../html FLASH_IN_PROJECT)

