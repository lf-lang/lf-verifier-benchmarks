# Remove the quotation marks
string(REPLACE "\"" "" stripped_source_path ${LF_SOURCE_DIRECTORY})

target_sources(${LF_MAIN_TARGET} PRIVATE
  ${stripped_source_path}/satellite_attitude_controller.c
  ${stripped_source_path}/synthetic_data.c
)
target_include_directories(${LF_MAIN_TARGET} PRIVATE
  ${stripped_source_path}
)